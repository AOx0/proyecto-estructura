use std::collections::HashMap;
use std::os::raw::*;
use std::ffi::CString;
use std::sync::{Mutex, Arc};
use std::thread::JoinHandle;
use std::time::Duration;
use mio::net::{TcpStream, TcpListener};
use mio::{Events, Interest, Poll, Token, event::Event, Registry};
use std::io::{Read, Write};
use std::str::from_utf8;

pub struct TcpServer {
    listener: TcpListener,
    poll: Poll,
    events: Events,
    connections: HashMap<Token, TcpStream>,
    response: HashMap<Token, String>,
    unique_token: Token,
    receiver: Arc<Mutex<bool>> 
}

static SERVER: Token = Token(0);

impl TcpServer {
    pub fn new_async(rx: Arc<Mutex<bool>>) -> TcpServer {
        let mut result = TcpServer { 
            listener: TcpListener::bind("127.0.0.1:9999".parse().unwrap()).expect("Failed to init listener"), 
            poll: Poll::new().expect("Failed to init poll"), 
            events: Events::with_capacity(128), 
            connections: HashMap::new(),
            response: HashMap::new(),
            unique_token: Token(SERVER.0 + 1),
            receiver: rx
        };

        result.poll.registry().register(&mut result.listener, Token(0), Interest::READABLE | Interest::WRITABLE).unwrap();
        
        result
    }

    pub fn new() -> TcpServer {
        TcpServer::new_async(Arc::new(Mutex::new(false)))
    }

    pub fn next(&mut self) {
            self.poll.poll(&mut self.events, None).unwrap();
            for event in self.events.iter() {
                match event.token() {
                    Token(0) => loop {
                        // Received an event for the TCP server socket, which
                        // indicates we can accept an connection.
                        let (mut connection, address) = match self.listener.accept() {
                            Ok((connection, address)) => (connection, address),
                            Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                                break;
                            }
                            Err(e) => {
                                panic!("{}", e);
                            }
                        };

                        println!("Accepted connection from: {}", address);

                        let token = next(&mut self.unique_token);
                        self.poll.registry().register(
                            &mut connection,
                            token,
                            Interest::READABLE.add(Interest::WRITABLE),
                        ).unwrap();

                        self.connections.insert(token, connection);
                    },
                    token => {
                        // Maybe received an event for a TCP connection.
                        let done = if let Some(connection) = self.connections.get_mut(&token) {
                            let response_handler = self.response.entry(token).or_insert("".to_owned());
                            handle_connection_event(self.poll.registry(), connection, event, response_handler).unwrap()
                        } else {
                            false
                        };
                        if done {
                            if let Some(mut connection) = self.connections.remove(&token) {
                                self.poll.registry().deregister(&mut connection).unwrap();
                            }
                        }
                    }
                }
            }
    }

    pub fn run_server(&mut self) {
        while *self.receiver.lock().unwrap() == false { 
            self.poll.poll(&mut self.events, Some(Duration::from_secs_f32(5.0))).unwrap();
            for event in self.events.iter() {
                match event.token() {
                    Token(0) => loop {
                        // Received an event for the TCP server socket, which
                        // indicates we can accept an connection.
                        let (mut connection, address) = match self.listener.accept() {
                            Ok((connection, address)) => (connection, address),
                            Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                                break;
                            }
                            Err(e) => {
                                panic!("{}", e);
                            }
                        };

                        println!("Accepted connection from: {}", address);

                        let token = next(&mut self.unique_token);
                        self.poll.registry().register(
                            &mut connection,
                            token,
                            Interest::READABLE.add(Interest::WRITABLE),
                        ).unwrap();

                        self.connections.insert(token, connection);
                    },
                    token => {
                        // Maybe received an event for a TCP connection.
                        let done = if let Some(connection) = self.connections.get_mut(&token) {
                            let response_handler = self.response.entry(token).or_insert("".to_owned());
                            handle_connection_event(self.poll.registry(), connection, event, response_handler).unwrap()
                        } else {
                            false
                        };
                        if done {
                            if let Some(mut connection) = self.connections.remove(&token) {
                                self.poll.registry().deregister(&mut connection).unwrap();
                            }
                        }
                    }
                }
            }
        } 
    }
}

struct RunTime {
    process: JoinHandle<()>,
    sender: Arc<Mutex<bool>>
}

#[no_mangle]
pub unsafe extern "C" fn start_server() -> *mut c_void {
    let cx: Arc<Mutex<bool>> = Arc::new(Mutex::new(false)); 
    let join = std::thread::spawn({
        let cx = Arc::clone(&cx);
        move || {
            let mut tcp = TcpServer::new_async(cx);
            tcp.run_server();
        }
    });

    let runtime = Box::new(RunTime {
        process: join,
        sender: cx
    });
    
    Box::into_raw(runtime) as *mut c_void

}

#[no_mangle]
pub unsafe extern "C" fn end_server(server: *mut c_void) {
    println!("Recovering raw pointer...");
    let runtime = Box::from_raw(server as *mut RunTime);
    println!("Sending end...");
    { 
        *runtime.sender.lock().unwrap() = true;
    }
    println!("Joining process...");
    runtime.process.join().unwrap();
    println!("Ending server!!");
}

/// Returns `true` if the connection is done.
fn handle_connection_event(
    registry: &Registry,
    connection: &mut TcpStream,
    event: &Event,
    data: &mut String
) -> std::io::Result<bool> {
    println!("{:?}", event);

    if event.is_readable() {
        let mut connection_closed = false;
        let mut received_data = vec![0; 4096];
        let mut bytes_read = 0;

        loop {
            match connection.read(&mut received_data[bytes_read..]) {
                Ok(0) => { connection_closed = true; break; }
                Ok(n) => {
                    bytes_read += n;
                    if bytes_read == received_data.len() {
                        received_data.resize(received_data.len() + 1024, 0);
                    }
                },
                Err(ref err) if would_block(err) => break,
                Err(ref err) if interrupted(err) => continue,
                Err(err) => return Err(err),
            }
        }

        if bytes_read != 0 {
            let received_data = &received_data[..bytes_read];
            if let Ok(str_buf) = from_utf8(received_data) {
                println!("Received data: {}", str_buf.trim_end());
                if str_buf.trim_end().contains("END") {
                    connection_closed = true;
                } else if str_buf.trim_end().contains("IN"){
                    registry.reregister(connection, event.token(), Interest::WRITABLE)?
                } else {
                    *data = str_buf.to_string();
                }
            } else {
                println!("Received (none UTF-8) data: {:?}", received_data);
            }
        }

        if connection_closed {
            println!("Connection closed");
            return Ok(true);
        }
    }

    if event.is_writable() {
        // We can (maybe) write to the connection.
        match connection.write(data.as_bytes()) {
            // We want to write the entire `DATA` buffer in a single go. If we
            // write less we'll return a short write error (same as
            // `io::Write::write_all` does).
            Ok(n) if n < data.len() => return Err(std::io::ErrorKind::WriteZero.into()),
            Ok(_) => {
                // After we've written something we'll reregister the connection
                // to only respond to readable events.
                registry.reregister(connection, event.token(), Interest::READABLE)?
            }
            // Would block "errors" are the OS's way of saying that the
            // connection is not actually ready to perform this I/O operation.
            Err(ref err) if would_block(err) => {}
            // Got interrupted (how rude!), we'll try again.
            Err(ref err) if interrupted(err) => {
                return handle_connection_event(registry, connection, event, data)
            }
            // Other errors we'll consider fatal.
            Err(err) => return Err(err),
        }
    }


    Ok(false)
}

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

#[no_mangle]
pub unsafe extern "C" fn print(value: *mut c_char) {
    let slice = CString::from_raw(value);
    println!("Hello from rust {}", slice.to_str().unwrap());

    std::mem::forget(slice);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let strin = CString::new("Hola").unwrap();
        unsafe { print(strin.as_ptr() as *mut  std::os::raw::c_char) };
    }

    #[test]
    fn server() {
        unsafe {
            {
                let server: *mut c_void = start_server();
                let mut buf = String::new();
                std::io::stdin().read_line(&mut buf).unwrap();
                end_server(server);
            }
        }
    }
}
