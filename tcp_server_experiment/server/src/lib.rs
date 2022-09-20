use mio::net::{TcpListener, TcpStream};
use mio::{event::Event, Events, Interest, Poll, Registry, Token};
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::io::{Read, Write};
use std::mem::swap;
use std::os::raw::c_char;
use std::ptr::null_mut;
use std::str::from_utf8;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread::{spawn, JoinHandle};
use std::time::Duration;

pub struct ConnectionState {
    stream: TcpStream,
    response: String,
    c_sender: Sender<String>,
    c_receiver: Receiver<String>,
    rs_sender: Sender<String>,
    rs_receiver: Receiver<String>
}

pub struct TcpServer {
    listener: TcpListener,
    poll: Poll,
    events: Events,
    channels: HashMap<Token, ConnectionState>,
    unique_token: Token,
    c_response_receiver: Receiver<String>,
    query_sender: Sender<String>,
    keep_running: Arc<Mutex<bool>>,
    threads: Vec<JoinHandle<()>>
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
        cont: Receiver<String>,
        msg: Sender<String>,
        still_alive: Arc<Mutex<bool>>,
    ) -> TcpServer {
        let mut result = TcpServer {
            listener: TcpListener::bind("127.0.0.1:9999".parse().unwrap())
                .expect("Failed to init listener"),
            poll: Poll::new().expect("Failed to init poll"),
            events: Events::with_capacity(128),
            channels: HashMap::new(),
            unique_token: Token(SERVER.0 + 1),
            keep_running: still_alive,
            c_response_receiver: cont,
            query_sender: msg,
            threads: vec![]
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
        while *self.keep_running.lock().unwrap() {
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

                        self.channels.insert(token, ConnectionState {
                            stream: connection,
                            response: "".to_string(),
                            c_sender,
                            c_receiver,
                            rs_sender,
                            rs_receiver
                        });
                    },
                    token => {
                        // Manejo de posible mensaje desde una conexión activa de TCP
                        let done = if let Some(state) = self.channels.get_mut(&token) {
                            // Manejamos cualquiera que sea el mensaje recibido.
                            Self::handle_connection_event(
                                state,
                                self.poll.registry(),
                                event,
                                &mut self.c_response_receiver,
                                &mut self.query_sender,
                            )
                            .unwrap()
                        } else {
                            false
                        };
                        if done {
                            if let Some(ConnectionState { stream: mut connection, .. }) = self.channels.remove(&token) {
                                self.poll.registry().deregister(&mut connection).unwrap();
                            }
                        }
                    }
                }
            }
        }
    }

    fn handle_connection_event(
        connection: &mut ConnectionState,
        registry: &Registry,
        event: &Event,
        cx: &mut Receiver<String>,
        rx: &mut Sender<String>,
    ) -> std::io::Result<bool> {
        //println!("{:?}", event);

        if event.is_readable() {
            let mut connection_closed = false;
            let mut received_data = vec![0; 4096];
            let mut bytes_read = 0;

            loop {
                match connection.stream.read(&mut received_data[bytes_read..]) {
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

                    // Send received data from tcp stream to c++
                    rx.send(str_buf.trim().to_owned()).unwrap();

                    if str_buf.trim_end().to_lowercase().contains("exit") {
                        connection_closed = true;
                    } else {
                        // Receive response from c++ runtime
                        let response = cx.recv().unwrap_or_else(|_| " ".to_owned());

                        connection.response = response;

                        // Register writable event to send response
                        registry.reregister(&mut connection.stream, event.token(), Interest::WRITABLE)?;
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
            let mut data = String::new();
            swap(&mut connection.response, &mut data);

            match connection.stream.write(data.as_bytes()) {
                Ok(n) if n < data.len() => return Err(std::io::ErrorKind::WriteZero.into()),
                Ok(_) => registry.reregister(&mut connection.stream, event.token(), Interest::READABLE)?,

                Err(ref err) if Self::would_block(err) => {}

                Err(ref err) if Self::interrupted(err) => {
                    return Self::handle_connection_event(connection, registry, event, cx, rx)
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
    /// Apuntador a el sender de continuar con el mensaje de respuesta
    pub continue_signal: *mut u8,
    /// Apuntador a el receiver de continuar con el mensaje recibido
    pub recv_signal: *mut u8,
    /// Apuntador a mutex que indica si el proceso sigue vivo
    pub stay_alive: *mut u8,
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
    let still_alive = Arc::new(Mutex::new(true));
    let (cx, rx): (Sender<String>, Receiver<String>) = channel();
    let cx = Box::into_raw(Box::new(cx));

    let (cx2, rx2): (Sender<String>, Receiver<String>) = channel();
    let rx2 = Box::into_raw(Box::new(rx2));

    let runtime = Box::into_raw(Box::new(spawn({
        let still = Arc::clone(&still_alive);
        || {
            let mut server = TcpServer::new(rx, cx2, still);
            server.run_server();
        }
    })));

    let still_alive = Box::into_raw(Box::new(still_alive));

    Tcp {
        runtime: runtime as *mut u8,
        continue_signal: cx as *mut u8,
        recv_signal: rx2 as *mut u8,
        stay_alive: still_alive as *mut u8,
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
