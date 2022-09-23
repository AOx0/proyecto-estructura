extern crate core;

use mio::net::{TcpListener, TcpStream};
use mio::{event::Event, Events, Interest, Poll, Registry, Token};
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::io::{Read, Write};
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::str::from_utf8;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::{Arc, Mutex, RwLock};
use std::thread::{spawn, JoinHandle};
use std::time::Duration;

pub struct ConnectionState {
    stream: Arc<Mutex<TcpStream>>,
    response: Arc<Mutex<String>>,
    c_sender: Arc<Mutex<Sender<String>>>,
    c_receiver: Arc<Mutex<Receiver<String>>>,
    rs_sender: Arc<Mutex<Sender<String>>>,
    rs_receiver: Arc<Mutex<Receiver<String>>>,
    is_active: Arc<Mutex<bool>>,
}

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
        msg: Sender<usize>,
        channels: Arc<RwLock<HashMap<Token, ConnectionState>>>,
    ) -> TcpServer {
        let mut result = TcpServer {
            listener: TcpListener::bind("127.0.0.1:9999".parse().unwrap())
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
        while !*END.read().unwrap() {
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

                        //println!("Accepted connection from: {}", address);

                        let token = Self::next(&mut self.unique_token);
                        self.poll
                            .registry()
                            .register(
                                &mut connection,
                                token,
                                Interest::READABLE.add(Interest::WRITABLE),
                            )
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
                            Self::handle_connection_event(
                                self.poll.registry().try_clone().unwrap(),
                                state,
                                event,
                                &mut self.query_sender,
                                &mut self.threads,
                            )
                            .unwrap()
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
    }

    fn handle_connection_event(
        registry: Registry,
        connection: &ConnectionState,
        event: &Event,
        rx: &mut Sender<usize>,
        threads: &mut HashMap<Token, JoinHandle<()>>,
    ) -> std::io::Result<bool> {
        //println!("{:?}", event);

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
                    //println!("Received data: {}", str_buf.trim_end());

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
                                // TODO: Stop receiving while processing

                                // Receive response from private channel
                                let receiver_guard = rs_receiver.lock().unwrap();
                                let response = loop {
                                    if !(*END.read().unwrap()) {
                                        match receiver_guard
                                            .recv_timeout(Duration::from_secs_f32(5.0))
                                        {
                                            Ok(value) => {
                                                break value;
                                            }
                                            Err(_) => {
                                                continue;
                                            }
                                        }
                                    } else {
                                        return;
                                    }
                                };

                                println!("{:?}", response);

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
                            println!("Joining past thread...");
                            a.join().unwrap();
                        }
                    }
                } else {
                    //println!("Received (none UTF-8) data: {:?}", received_data);
                }
            }

            if connection_closed {
                //println!("Connection closed");
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
        println!("        Joining threads...");
        for t in self.threads.into_iter() {
            println!("            Joining thread...");
            t.1.join().expect("TODO: panic message");
        }
    }
}

// Bridge

#[repr(C)]
pub struct Tcp {
    /// Apuntador al tiempo de ejecución del servidor (JoinHandle<()>)
    pub runtime: *mut u8,
    /// Apuntador a el receiver de continuar con el mensaje recibido
    pub recv_signal: *mut u8,
    /// Apuntador al diccionario de estado de canales
    pub channels: *mut u8,
}

#[repr(C)]
pub struct Shared {
    // Pointer to an Allocated value
    value: *mut u8,
    // Type of the value
    typ: u8,
    // Is null?
    null: bool,
    // Token
    token: usize,
}

impl Drop for Shared {
    fn drop(&mut self) {
        if !self.null {
            unsafe {
                match self.typ {
                    1 => {
                        // Drop CString
                        let _ = CString::from_raw(self.value as *mut i8);
                    }
                    _ => {
                        println!("Wrong type u8 descriptor {} for {:p}", self.typ, self.value)
                    }
                }
            }
        }
    }
}

///# Safety
///
#[no_mangle]
pub unsafe extern "C" fn communicate(state: &mut Tcp, shared: &Shared, msg: *mut c_char) {
    let msg = CStr::from_ptr(msg);

    match msg.to_str() {
        Ok(value) => {
            let channels =
                Arc::from_raw(state.channels as *const RwLock<HashMap<Token, ConnectionState>>);

            let result = {
                let mut channel = channels.write().unwrap();
                let channel_result = channel.get_mut(&Token(shared.token));

                if channel_result.is_none() {
                    return;
                }

                let channel = channel_result.unwrap().c_sender.lock().unwrap();
                channel.send(value.to_owned())
            };

            state.channels = Arc::into_raw(channels) as *mut u8;

            match result {
                Ok(_) => {}
                Err(error) => {
                    println!("Failed to send trough channel: {:?}", error);
                }
            }
        }
        Err(_) => {
            println!(
                "Something bad happened while converting to string {:?}",
                msg
            )
        }
    }
}

///# Safety
///
#[no_mangle]
pub unsafe extern "C" fn receive(state: &mut Tcp) -> Shared {
    let receiver = Arc::from_raw(state.recv_signal as *const Receiver<usize>);
    let token = loop {
        if !(*END.read().unwrap()) {
            match receiver.recv_timeout(Duration::from_secs_f32(5.0)) {
                Ok(value) => {
                    break value;
                }
                Err(_) => {
                    continue;
                }
            }
        } else {
            return Shared {
                null: true,
                value: null_mut::<u8>(),
                typ: 0,
                token: 0,
            };
        }
    };

    state.recv_signal = Arc::into_raw(receiver) as *mut u8;

    let channels = Arc::from_raw(state.channels as *const RwLock<HashMap<Token, ConnectionState>>);

    let result = {
        let mut channel = channels.write().unwrap();
        let channel = channel
            .get_mut(&Token(token))
            .unwrap()
            .c_receiver
            .lock()
            .unwrap();
        loop {
            if !(*END.read().unwrap()) {
                match channel.recv_timeout(Duration::from_secs_f32(5.0)) {
                    Ok(value) => {
                        break value;
                    }
                    Err(_) => {
                        continue;
                    }
                }
            } else {
                return Shared {
                    null: true,
                    value: null_mut::<u8>(),
                    typ: 0,
                    token: 0,
                };
            }
        }
    };

    state.channels = Arc::into_raw(channels) as *mut u8;

    match CString::new(result) {
        Ok(value) => {
            let v = CString::into_raw(value);
            Shared {
                null: false,
                value: v as *mut u8,
                typ: 1,
                token,
            }
        }
        Err(err) => {
            println!("Failed to make CString: {:?}", err);
            Shared {
                null: true,
                value: null_mut::<u8>(),
                typ: 0,
                token: 0,
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn start() -> Tcp {
    println!("Starting TcpServer 0.1.13 ...");

    // Creamos el canal de notificación principal
    let (cx2, rx2): (Sender<usize>, Receiver<usize>) = channel();
    let rx2 = Arc::into_raw(Arc::new(rx2));

    // Creamos el diccionario de estado para las conexiones establecidas
    let channels = Arc::new(RwLock::new(HashMap::new()));

    // Spawn del server
    let runtime = Arc::into_raw(Arc::new(spawn({
        let channels = Arc::clone(&channels);
        || {
            let mut server = TcpServer::new(cx2, channels);
            server.run_server();
            println!("    Stopping server...");
            server.stop();
        }
    })));

    let channels = Arc::into_raw(channels);

    Tcp {
        runtime: runtime as *mut u8,
        recv_signal: rx2 as *mut u8,
        channels: channels as *mut u8,
    }
}

#[no_mangle]
pub extern "C" fn stop(state: Tcp) {
    println!("Finishing from rust...");

    println!("    Setting end rwlock to true...");
    match END.write() {
        Ok(mut value) => {
            *value = true;
        }
        Err(err) => {
            println!("Failed to lock Mutex: {:?}", err);
        }
    }

    println!("    Getting runtime...");
    let runtime =
        unsafe { Arc::try_unwrap(Arc::from_raw(state.runtime as *const JoinHandle<()>)).unwrap() };
    let join = runtime.join();

    println!("    Waiting for runtime...");
    match join {
        Ok(_) => {
            println!("        Done!");
        }
        Err(err) => {
            println!("Failed to join Rust Runtime: {:?}", err)
        }
    }

    unsafe {
        println!("    Dropping channels...");
        let _ = Arc::from_raw(state.channels as *const RwLock<HashMap<Token, ConnectionState>>);
        println!("    Dropping recv_signal...");
        //let _ = Arc::from_raw(state.recv_signal as *const Receiver<usize>);
    }
    println!("Done from rust!");
}

#[no_mangle]
extern "C" fn kill_sign() {
    println!("    Setting end rwlock to true...");
    match END.write() {
        Ok(mut value) => {
            *value = true;
        }
        Err(err) => {
            println!("Failed to lock Mutex: {:?}", err);
        }
    }
}

#[no_mangle]
extern "C" fn drop_shared(s: Shared) {
    drop(s);
}
