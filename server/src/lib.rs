extern crate alloc;

use alloc::ffi::CString;
use mio::net::{TcpListener, TcpStream};
use mio::{event::Event, Events, Interest, Poll, Registry, Token};
use std::collections::HashMap;
use std::ffi::{c_void, CStr};
use std::io::{Read, Write};
use std::mem::swap;
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::str::from_utf8;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::{Arc, Mutex, RwLock};
use std::thread::{spawn, JoinHandle};
use std::time::Duration;

/// Each client is represented by a `ConnectionState` struct.
///
/// Everything is stored in Mutexes to allow for concurrent access.
/// We do this because we want ot be able to access this struct from the C++ runtime, which is
/// it may be accessed in any point of time.
pub struct ConnectionState {
    /// The `TcpStream` of the client for we to read and write.
    pub stream: Arc<Mutex<TcpStream>>,
    /// The answer to the query.
    pub response: Arc<Mutex<String>>,
    ///  This sends messages to the client from c code to the `rs_receiver`
    pub c_sender: Arc<Mutex<Sender<String>>>,
    /// This receives messages from the client in c code to the `rs_sender`
    pub c_receiver: Arc<Mutex<Receiver<String>>>,
    /// This sends messages to the client from rust code received from the TcpStream
    pub rs_sender: Arc<Mutex<Sender<String>>>,
    /// This receives messages from the client in rust code from the `c_sender`.
    pub rs_receiver: Arc<Mutex<Receiver<String>>>,
    /// We store if the client has a query in progress. If it does, we
    /// don't want to read from the TcpStream until the query is finished.
    pub is_active: Arc<Mutex<bool>>,
}

/// Holds all connections and information of the tcp server.
pub struct TcpServer {
    listener: TcpListener,
    poll: Poll,
    events: Events,
    channels: Arc<RwLock<HashMap<Token, ConnectionState>>>,
    unique_token: Token,
    query_sender: Sender<usize>,
    threads: HashMap<Token, JoinHandle<()>>,
}

static SERVER: Token = Token(0);
static END: RwLock<bool> = RwLock::new(false);

macro_rules! try_op {
    [$action:expr, On error: $return_var:expr] => {
        match $action {
            Ok(s) => s,
            Err(_) => $return_var,
        }
    };
}

macro_rules! runtime_is_on {
    [On error: $value:expr] => {
        loop {
            if let Ok(end) = END.read() {
                break !(*end);
            }
            $value;
        }
    };
}

macro_rules! get_message {
    ($receiver:expr, On end: $ret:expr) => {
        {
            loop {
                if runtime_is_on![On error: $ret] {
                    if let Ok(value) = $receiver.recv_timeout(Duration::from_secs_f32(5.0)) {
                        break value;
                    } else {
                        continue;
                    }
                }

                 $ret;
            }
        }
    };
}

impl TcpServer {
    fn would_block(err: &std::io::Error) -> bool {
        err.kind() == std::io::ErrorKind::WouldBlock
    }

    fn interrupted(err: &std::io::Error) -> bool {
        err.kind() == std::io::ErrorKind::Interrupted
    }

    fn next(current: &mut Token) -> Token {
        let next = current.0;
        current.0 += 1;
        Token(next)
    }

    pub fn new(
        ip: String,
        port: String,
        msg: Sender<usize>,
        channels: Arc<RwLock<HashMap<Token, ConnectionState>>>,
    ) -> TcpServer {
        let mut result = TcpServer {
            listener: TcpListener::bind(format!("{ip}:{port}").parse().unwrap())
                .expect("Failed to init listener"),
            poll: Poll::new().expect("Failed to init poll"),
            events: Events::with_capacity(1024),
            channels,
            unique_token: Token(SERVER.0 + 1),
            query_sender: msg,
            threads: HashMap::new(),
        };

        result
            .poll
            .registry()
            .register(
                &mut result.listener,
                Token(0),
                Interest::READABLE | Interest::WRITABLE,
            )
            .unwrap();
        result
    }

    pub fn run_server(&mut self) {
        while runtime_is_on!(On error: ()) {
            self.poll
                .poll(&mut self.events, Some(Duration::from_secs_f32(5.0)))
                .unwrap();

            for event in self.events.iter() {
                match event.token() {
                    Token(0) => loop {
                        // Evento de socket para el server, aceptamos la conexión nueva y la registramos
                        let (mut connection, _) = match self.listener.accept() {
                            Ok((connection, address)) => (connection, address),
                            Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                                break;
                            }
                            Err(e) => {
                                panic!("{}", e);
                            }
                        };

                        ////println!("Accepted connection from: {}", address);

                        let token = Self::next(&mut self.unique_token);
                        self.poll
                            .registry()
                            .register(&mut connection, token, Interest::READABLE)
                            .unwrap();

                        let (c_sender, rs_receiver) = channel();
                        let (rs_sender, c_receiver) = channel();

                        self.channels.write().unwrap().insert(
                            token,
                            ConnectionState {
                                stream: Arc::new(Mutex::new(connection)),
                                response: Arc::new(Mutex::new("".to_string())),
                                c_sender: Arc::new(Mutex::new(c_sender)),
                                c_receiver: Arc::new(Mutex::new(c_receiver)),
                                rs_sender: Arc::new(Mutex::new(rs_sender)),
                                rs_receiver: Arc::new(Mutex::new(rs_receiver)),
                                is_active: Arc::new(Mutex::new(false)),
                            },
                        );
                    },
                    token => {
                        // Manejo de posible mensaje desde una conexión activa de TCP
                        let done = if let Some(state) = self.channels.read().unwrap().get(&token) {
                            // Manejamos cualquiera que sea el mensaje recibido.
                            match Self::handle_connection_event(
                                self.poll.registry().try_clone().unwrap(),
                                state,
                                event,
                                &mut self.query_sender,
                                &mut self.threads,
                            ) {
                                Ok(value) => value,
                                Err(_) => true,
                            }
                        } else {
                            false
                        };
                        if done {
                            if let Some(ConnectionState {
                                stream: connection, ..
                            }) = self.channels.write().unwrap().remove(&token)
                            {
                                self.poll
                                    .registry()
                                    .deregister(&mut *connection.lock().unwrap())
                                    .unwrap();
                            }
                        }
                    }
                }
            }
        }

        for (token, state) in self.channels.read().unwrap().iter() {
            let _ = self
                .poll
                .registry()
                .deregister(&mut *state.stream.lock().unwrap());
            self.threads.remove(token);
        }
    }

    fn handle_connection_event(
        registry: Registry,
        connection: &ConnectionState,
        event: &Event,
        rx: &mut Sender<usize>,
        threads: &mut HashMap<Token, JoinHandle<()>>,
    ) -> std::io::Result<bool> {
        if event.is_readable() && !*connection.is_active.lock().unwrap() {
            let mut connection_closed = false;
            let mut received_data = vec![0; 4096];
            let mut bytes_read = 0;

            loop {
                match connection
                    .stream
                    .lock()
                    .unwrap()
                    .read(&mut received_data[bytes_read..])
                {
                    Ok(0) => {
                        connection_closed = true;
                        break;
                    }
                    Ok(n) => {
                        bytes_read += n;
                        if bytes_read == received_data.len() {
                            received_data.resize(received_data.len() + 1024, 0);
                        }
                    }
                    Err(ref err) if Self::would_block(err) => break,
                    Err(ref err) if Self::interrupted(err) => continue,
                    Err(err) => return Err(err),
                }
            }

            if bytes_read != 0 {
                *connection.is_active.lock().unwrap() = true;
                let received_data = &received_data[..bytes_read];
                if let Ok(str_buf) = from_utf8(received_data) {
                    if str_buf.trim_end().to_lowercase().contains("exit") {
                        connection_closed = true;
                    } else {
                        // Send received data notification from tcp stream to c++
                        rx.send(event.token().0).unwrap();

                        // Send message from private channel
                        connection
                            .rs_sender
                            .lock()
                            .unwrap()
                            .send(str_buf.trim_end().to_owned())
                            .unwrap();

                        // Receive response from c++ runtime
                        let t = spawn({
                            let token = event.token();
                            let rs_receiver = Arc::clone(&connection.rs_receiver);
                            let future_response = Arc::clone(&connection.response);
                            let connection = Arc::clone(&connection.stream);
                            move || {
                                // Receive response from private channel
                                let response =
                                    get_message!(rs_receiver.lock().unwrap(), On end: return);

                                *future_response.lock().unwrap() = response;

                                // Register writable event to send response
                                registry
                                    .reregister(
                                        &mut *connection.lock().unwrap(),
                                        token,
                                        Interest::WRITABLE,
                                    )
                                    .unwrap();
                            }
                        });

                        let a = threads.insert(event.token(), t);
                        if let Some(a) = a {
                            a.join().unwrap();
                        }
                    }
                }
            }

            if connection_closed {
                return Ok(true);
            }
        } else if event.is_writable() {
            let data = connection.response.lock().unwrap().clone();
            let write = connection.stream.lock().unwrap().write(data.as_bytes());

            match write {
                Ok(n) if n < data.len() => return Err(std::io::ErrorKind::WriteZero.into()),
                Ok(_) => {
                    *connection.is_active.lock().unwrap() = false;
                    registry.reregister(
                        &mut *connection.stream.lock().unwrap(),
                        event.token(),
                        Interest::READABLE,
                    )?
                }

                Err(ref err) if Self::would_block(err) => {}
                Err(ref err) if Self::interrupted(err) => {
                    return Self::handle_connection_event(registry, connection, event, rx, threads)
                }
                Err(err) => return Err(err),
            }
        }

        Ok(false)
    }

    fn stop(self) {
        for t in self.threads.into_iter() {
            t.1.join().expect("TODO: panic message");
        }
    }
}

/// C binding to share data between c++ and rust
#[repr(C)]
pub struct Shared {
    /// Pointer to an Allocated value of type `typ`
    pub value: *mut u8,
    /// Is this struct null because an error happened?
    pub null: bool,
    /// Token of the tcp connection that sent the message
    pub token: usize,
}

impl Shared {
    fn null() -> Self {
        Shared {
            null: true,
            value: null_mut::<u8>(),
            token: 0,
        }
    }
}

impl Drop for Shared {
    fn drop(&mut self) {
        if !self.null {
            unsafe {
                let _ = CString::from_raw(self.value as *mut i8);
            }
        }
    }
}

///# Safety
///
#[no_mangle]
pub unsafe extern "C" fn communicate(
    state_ptr: *mut c_void,
    shared: &Shared,
    msg: *const c_char,
) -> bool {
    let state = (state_ptr as *mut Tcp).as_mut().unwrap();

    if let Ok(mut channel) = state.channels.write() {
        if let Some(channel) = channel.get_mut(&Token(shared.token)) {
            if let Ok(channel) = channel.c_sender.lock() {
                return channel
                    .send(try_op![CStr::from_ptr(msg).to_str(), On error: return false].to_owned())
                    .is_ok();
            }
        }
    }

    return false;
}

///# Safety
///
#[no_mangle]
pub unsafe extern "C" fn receive(state_ptr: *mut c_void) -> Shared {
    let state = (state_ptr as *mut Tcp).as_mut().unwrap();
    let token = get_message!(*state.recv_signal.lock().unwrap(), On end: return Shared::null());

    let result = loop {
        if let Ok(mut channels) = state.channels.write() {
            if let Some(channel) = channels.get_mut(&Token(token)) {
                if let Ok(channel) = channel.c_receiver.lock() {
                    break get_message!(channel, On end: return Shared::null());
                }
            }
        }
        return Shared::null();
    };

    return Shared {
        null: false,
        value: (try_op!(CString::new(result), On error: return Shared::null()).into_raw())
            as *mut u8,
        token,
    };
}

/// C binding to share the TCP server information with C++.
pub struct Tcp {
    /// Pointer to the servers' runtime (JoinHandle<()>)
    pub runtime: Option<JoinHandle<()>>,
    /// Pointer to a signal receiver to get notification about new messages
    pub recv_signal: Arc<Mutex<Receiver<usize>>>,
    /// Pointer to the hashmap of the servers' connections
    pub channels: Arc<RwLock<HashMap<Token, ConnectionState>>>,
}

#[no_mangle]
pub extern "C" fn start(ip: *const c_char, port: *const c_char) -> *mut c_void {
    let ip = try_op![unsafe { CStr::from_ptr(ip).to_str() }, On error: return null_mut()];
    let port = try_op![unsafe { CStr::from_ptr(port).to_str() }, On error: return null_mut()];

    // Creamos el canal de notificación principal
    let (cx2, rx2): (Sender<usize>, Receiver<usize>) = channel();
    let rx2 = Arc::new(Mutex::new(rx2));

    // Creamos el diccionario de estado para las conexiones establecidas
    let channels = Arc::new(RwLock::new(HashMap::new()));

    // Spawn del server
    let runtime = spawn({
        let channels = Arc::clone(&channels);
        || {
            let mut server = TcpServer::new(ip.to_owned(), port.to_owned(), cx2, channels);
            server.run_server();
            server.stop();
        }
    });

    let pointer = Box::new(Tcp {
        runtime: Some(runtime),
        recv_signal: rx2,
        channels,
    });

    Box::into_raw(pointer) as *mut c_void
}

#[no_mangle]
pub extern "C" fn stop(state: *mut c_void) {
    let mut state = unsafe { Box::from_raw(state as *mut Tcp) };

    if let Ok(mut value) = END.write() {
        *value = true;
    }

    let mut runtime = None;
    swap(&mut state.runtime, &mut runtime);
    runtime.unwrap().join().unwrap();
}

#[no_mangle]
extern "C" fn kill_sign() {
    if let Ok(mut value) = END.write() {
        *value = true;
    }
}

#[no_mangle]
extern "C" fn drop_shared(s: Shared) {
    drop(s);
}
