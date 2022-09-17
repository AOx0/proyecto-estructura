use std::collections::HashMap;
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

    pub fn new() -> TcpServer {
        let mut result = TcpServer {
            listener: TcpListener::bind("127.0.0.1:9999".parse().unwrap()).expect("Failed to init listener"),
            poll: Poll::new().expect("Failed to init poll"),
            events: Events::with_capacity(128),
            connections: HashMap::new(),
            response: HashMap::new(),
            unique_token: Token(SERVER.0 + 1),
        };

        result.poll.registry().register(&mut result.listener, Token(0), Interest::READABLE | Interest::WRITABLE).unwrap();
        result
    }

    pub fn run_server(&mut self) {
        loop {
            self.poll.poll(&mut self.events, Some(Duration::from_secs_f32(5.0))).unwrap();
            for event in self.events.iter() {
                match event.token() {
                    Token(0) => loop {
                        // Evento de socket para el server, aceptamos la conexión nueva y la registramos
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

                        let token = Self::next(&mut self.unique_token);
                        self.poll.registry().register(
                            &mut connection,
                            token,
                            Interest::READABLE.add(Interest::WRITABLE),
                        ).unwrap();

                        self.connections.insert(token, connection);
                    },
                    token => {
                        // Manejo de posible mensaje desde una conexión activa de TCP
                        let done = if let Some(connection) = self.connections.get_mut(&token) {
                            let response_handler = self.response.entry(token).or_insert("".to_owned());
                            // Manejamos cualquiera que sea el mensaje recibido.
                            Self::handle_connection_event(self.poll.registry(), connection, event, response_handler).unwrap()
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
                    Err(ref err) if Self::would_block(err) => break,
                    Err(ref err) if Self::interrupted(err) => continue,
                    Err(err) => return Err(err),
                }
            }

            if bytes_read != 0 {
                let received_data = &received_data[..bytes_read];
                if let Ok(str_buf) = from_utf8(received_data) {
                    println!("Received data: {}", str_buf.trim_end());
                    if str_buf.trim_end().to_lowercase().contains("exit") {
                        connection_closed = true;
                    } else {
                        //TODO: Inyectar la espera de señal de completado con el proceso desde C++
                        *data = str_buf.to_string();
                        registry.reregister(connection, event.token(), Interest::WRITABLE)?;
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
            match connection.write(data.as_bytes()) {

                Ok(n) if n < data.len() => return Err(std::io::ErrorKind::WriteZero.into()),
                Ok(_) => {
                    registry.reregister(connection, event.token(), Interest::READABLE)?
                }

                Err(ref err) if Self::would_block(err) => {}

                Err(ref err) if Self::interrupted(err) => {
                    return Self::handle_connection_event(registry, connection, event, data)
                }

                Err(err) => return Err(err),
            }
        }

        Ok(false)
    }

}