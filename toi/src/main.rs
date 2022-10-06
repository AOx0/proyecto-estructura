use std::{io::{Write, Read}, process::exit};
use text_io::*;
use clap::Parser;
use mio::net::TcpStream;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// The ip to connect to
    #[arg(short, long, default_value = "127.0.0.1")]
    ip: String,
    /// The port to connect to
    #[arg(short, long, default_value = "9999")]
    port: String
}

fn main()  {
    let Args { ip, port } = Args::parse();

    let address = format!("{}:{}", ip, port);
    let address = address.parse().unwrap();
   
    'main : loop {
        let stream = TcpStream::connect(address);

        if stream.is_err() {
            println!("Error while connecting to tcp: {}", stream.unwrap_err().to_string());
            exit(1);
        }

        let mut stream = stream.unwrap();

        if stream.write("Hey".as_bytes()).is_err() {
            continue;
        }

        if stream.write("Hey".as_bytes()).is_err() {
            continue;
        }
        
        loop {
            let mut buf = [0; 512*4];

            let message: Result<String, _> = try_read!("{}");

            if let Ok(msg) = message {
                if !msg.is_ascii() {
                    continue;
                }

                let bytes = msg.as_bytes();

                // Send message

                if let Ok(_) = stream.write(bytes) {

                    if msg == "exit" {
                        break 'main;
                    }
                    // Receive response
                    if let Ok(_) = stream.read(&mut buf) {

                        let response = String::from_utf8(buf.to_vec());

                        if response.is_ok() {
                            print!("{}", response.unwrap());
                        }
                    } else {
                        println!("Error while reading from TcpStream. Is the server on?");
                    }
                } else {
                    println!("Error while writing to TcpStream. Is the server on?");
                    continue 'main;
                }
            }
        }
    }
}
