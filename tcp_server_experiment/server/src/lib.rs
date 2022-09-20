extern crate core;


use mio::net::{TcpListener, TcpStream};
use mio::{event::Event, Events, Interest, Poll, Token};
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::io::{Read, Write};
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::str::from_utf8;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread::{spawn, JoinHandle};
use std::time::Duration;

pub struct ConnectionState {
    stream: Arc<Mutex<TcpStream>>,
    response: Arc<Mutex<String>>,
    c_sender: Arc<Mutex<Sender<String>>>,
    c_receiver: Arc<Mutex<Receiver<String>>>,
    rs_sender: Arc<Mutex<Sender<String>>>,
    rs_receiver: Arc<Mutex<Receiver<String>>>
}

pub struct TcpServer {
    listener: TcpListener,
    poll: Arc<Mutex<Poll>>,
    events: Events,
    channels: Arc<Mutex<HashMap<Token, ConnectionState>>>,
    queue: Arc<Mutex<Vec<u8>>>,
    unique_token: Token,
    query_sender: Sender<String>,
    keep_running: Arc<Mutex<bool>>,
    threads: Vec<JoinHandle<()>>,
}

static SERVER: Token = Token(0);


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
        msg: Sender<String>,
        still_alive: Arc<Mutex<bool>>,
        channels: Arc<Mutex<HashMap<Token, ConnectionState>>>,
        queue: Arc<Mutex<Vec<u8>>>
    ) -> TcpServer {
        let mut result = TcpServer {
            listener: TcpListener::bind("127.0.0.1:9999".parse().unwrap())
                .expect("Failed to init listener"),
            poll: Arc::new(Mutex::new(Poll::new().expect("Failed to init poll"))),
            events: Events::with_capacity(1024),
            channels,
            queue,
            unique_token: Token(SERVER.0 + 1),
            keep_running: still_alive,
            query_sender: msg,
            threads: vec![],
        };

        result
            .poll
            .lock()
            .unwrap()
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
        while *self.keep_running.lock().unwrap() {
            self.poll
                .lock()
                .unwrap()
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
                          .lock().unwrap()
                            .registry()
                            .register(
                                &mut connection,
                                token,
                                Interest::READABLE.add(Interest::WRITABLE),
                            )
                            .unwrap();

                        let (c_sender, rs_receiver) = channel();
                        let (rs_sender, c_receiver) = channel();

                        self.channels.lock().unwrap().insert(token, ConnectionState {
                            stream: Arc::new(Mutex::new(connection)),
                            response: Arc::new(Mutex::new("".to_string())),
                            c_sender: Arc::new(Mutex::new(c_sender)),
                            c_receiver: Arc::new(Mutex::new(c_receiver)),
                            rs_sender: Arc::new(Mutex::new(rs_sender)),
                            rs_receiver: Arc::new(Mutex::new(rs_receiver))
                        });
                    },
                    token => {
                        // Manejo de posible mensaje desde una conexión activa de TCP
                        let done = if let Some(state) = self.channels.lock().unwrap().get_mut(&token) {
                            // Manejamos cualquiera que sea el mensaje recibido.
                            Self::handle_connection_event(
                                Arc::clone(&self.poll),
                                state,
                                event,
                                &mut self.query_sender,
                                &mut self.threads,
                                Arc::clone(&self.queue)
                            )
                            .unwrap()
                        } else {
                            false
                        };
                        if done {
                            if let Some(ConnectionState { stream: connection, .. }) = self.channels.lock().unwrap().remove(&token) {
                                self.poll.lock().unwrap().registry().deregister(&mut *connection.lock().unwrap()).unwrap();
                            }
                        }
                    }
                }
            }
        }
    }

    fn handle_connection_event(
        poll: Arc<Mutex<Poll>>,
        connection: &mut ConnectionState,
        event: &Event,
        rx: &mut Sender<String>,
        threads: &mut Vec<JoinHandle<()>>,
        queue: Arc<Mutex<Vec<u8>>>
    ) -> std::io::Result<bool> {
        //println!("{:?}", event);

        if event.is_readable() {
            let mut connection_closed = false;
            let mut received_data = vec![0; 4096];
            let mut bytes_read = 0;

            loop {
                match connection.stream.lock().unwrap().read(&mut received_data[bytes_read..]) {
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
                let received_data = &received_data[..bytes_read];
                if let Ok(str_buf) = from_utf8(received_data) {
                    //println!("Received data: {}", str_buf.trim_end());

                    // Send received data notification from tcp stream to c++
                    rx.send(str_buf.trim().to_owned()).unwrap();

                    if str_buf.trim_end().to_lowercase().contains("exit") {
                        connection_closed = true;
                    } else {
                        // Receive response from c++ runtime
                        let t = spawn({
                            let registry = Arc::clone(&poll);
                            let token = event.token();
                            let rs_receiver = Arc::clone(&connection.rs_receiver);
                            let future_response = Arc::clone(&connection.response);
                            let connection = Arc::clone(&connection.stream);
                            move || {
                            // Receive response from private channel
                            let receiver_guard = rs_receiver.lock().unwrap();
                            let response = receiver_guard.recv().unwrap();

                            *future_response.lock().unwrap() = response;

                            // Register writable event to send response
                            registry.lock().unwrap().registry().reregister(&mut *connection.lock().unwrap(), token, Interest::WRITABLE).unwrap();
                        }});

                        threads.push(t);
                    }
                } else {
                    //println!("Received (none UTF-8) data: {:?}", received_data);
                }
            }

            if connection_closed {
                //println!("Connection closed");
                return Ok(true);
            }
        }

        if event.is_writable() {
            let data = connection.response.lock().unwrap().clone();

            let write = connection.stream.lock().unwrap().write(data.as_bytes());

            match write {
                Ok(n) if n < data.len() => return Err(std::io::ErrorKind::WriteZero.into()),
                Ok(_) => poll.lock().unwrap().registry().reregister(&mut *connection.stream.lock().unwrap(), event.token(), Interest::READABLE)?,

                Err(ref err) if Self::would_block(err) => {}

                Err(ref err) if Self::interrupted(err) => {
                    return Self::handle_connection_event(poll, connection, event, rx, threads, queue)
                }

                Err(err) => return Err(err),
            }
        }

        Ok(false)
    }
}

// Bridge

#[repr(C)]
pub struct Tcp {
    /// Apuntador al tiempo de ejecución del servidor (JoinHandle<()>)
    pub runtime: *mut u8,
    /// Apuntador a el receiver de continuar con el mensaje recibido
    pub recv_signal: *mut u8,
    /// Apuntador a mutex que indica si el proceso sigue vivo
    pub stay_alive: *mut u8,
    /// Apuntador al diccionario de estado de canales
    pub channels: *mut u8,
    /// Apuntador a una cola de Token por escuchar
    pub queue: *mut u8
}

#[repr(C)]
pub struct Shared {
    // Pointer to an Allocated value
    value: *mut u8,
    // Type of the value
    typ: u8,
    // Is null?
    null: bool,
}

#[no_mangle]
pub extern "C" fn drop_shared(v: Shared) {
    if !v.null {
        unsafe {
            match v.typ {
                1 => {
                    // Drop String
                    let _ = CString::from_raw(v.value as *mut i8);
                }
                _ => {
                    println!("Wrong type u8 descriptor {} for {:p}", v.typ, v.value)
                }
            }
        }
    }
}

///# Safety
///
#[no_mangle]
pub unsafe extern "C" fn communicate(state: &mut Tcp, msg: *mut c_char) {
    let msg = CStr::from_ptr(msg);

    match msg.to_str() {
        Ok(value) => {
            let sender = { Box::from_raw(state.continue_signal as *mut Sender<String>) };

            match sender.send(value.to_owned()) {
                Ok(_) => {
                    state.continue_signal = Box::into_raw(sender) as *mut u8;
                }
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
    let x = Box::from_raw(state.recv_signal as *mut Receiver<String>);
    match x.recv() {
        Ok(value) => {
            state.recv_signal = Box::into_raw(x) as *mut u8;
            match CString::new(value) {
                Ok(value) => {
                    let v = CString::into_raw(value);
                    Shared {
                        null: false,
                        value: v as *mut u8,
                        typ: 1,
                    }
                }
                Err(err) => {
                    println!("Failed to make CString: {:?}", err);
                    Shared {
                        null: true,
                        value: null_mut::<u8>(),
                        typ: 0,
                    }
                }
            }
        }
        Err(err) => {
            state.recv_signal = Box::into_raw(x) as *mut u8;
            println!(
                "Something wrong happened while reading from channel: {:?}",
                err
            );
            Shared {
                null: true,
                value: null_mut::<u8>(),
                typ: 0,
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn start() -> Tcp {
    // Creamos el mutex que mantiene corriendo el servidor
    let still_alive = Arc::new(Mutex::new(true));

    // Creamos el canal de notificación principal
    let (cx2, rx2): (Sender<String>, Receiver<String>) = channel();
    let rx2 = Box::into_raw(Box::new(rx2));

    // Creamos el diccionario de estado para las conexiones establecidas
    let channels = Arc::new(Mutex::new(HashMap::new()));

    // Creamos la cola de espera de mensajes
    let queue = Arc::new(Mutex::new(Vec::new()));

    // Spawn del server
    let runtime = Box::into_raw(Box::new(spawn({
        let channels = Arc::clone(&channels);
        let queue = Arc::clone(&queue);
        let still = Arc::clone(&still_alive);
        || {
            let mut server = TcpServer::new(cx2, still, channels, queue);
            server.run_server();
            for t in server.threads {
                t.join().unwrap();
            }
        }
    })));

    let still_alive = Box::into_raw(Box::new(still_alive));
    let channels = Box::into_raw(Box::new(channels));
    let queue = Box::into_raw(Box::new(queue));

    Tcp {
        runtime: runtime as *mut u8,
        recv_signal: rx2 as *mut u8,
        stay_alive: still_alive as *mut u8,
        channels: channels as *mut u8,
        queue: queue as *mut u8
    }
}

#[no_mangle]
pub extern "C" fn stop(state: Tcp) {
    let still_alive = unsafe { Box::from_raw(state.stay_alive as *mut Arc<Mutex<bool>>) };
    match still_alive.lock() {
        Ok(mut value) => {
            *value = false;
        }
        Err(err) => {
            println!("Failed to lock Mutex: {:?}", err);
        }
    }

    let runtime = unsafe { Box::from_raw(state.runtime as *mut JoinHandle<()>) };

    match runtime.join() {
        Ok(_) => {}
        Err(err) => {
            println!("Failed to join Rust Runtime: {:?}", err)
        }
    }

    // Drop channels
    unsafe {
        Box::from_raw(state.recv_signal as *mut Receiver<String>);
        Box::from_raw(state.continue_signal as *mut Sender<String>);
    }
}
