use clap::Parser;
use std::net::TcpStream;
use std::{
    io::{Read, Write},
    process::exit,
};
use text_io::*;

/// DB Client
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// The ip to connect to
    #[arg(required = true)]
    ip: String,
    /// The port to connect to
    #[arg(required = true)]
    port: String,
}

fn run() -> Result<(), Box<dyn std::error::Error>> {
    let Args { ip, port } = Args::parse();

    let mut stream = TcpStream::connect(format!("{ip}:{port}"))?;

    loop {
        let mut buf = vec![0; 1024 * 2];
        let msg: String = try_read!("{}\n")?;

        if !msg.is_ascii() {
            println!("Message must be ascii");
            continue;
        }

        stream.write_all(msg.as_bytes())?;

        if msg == "exit" || msg == "stop" {
            break;
        }

        let n = stream.read(&mut buf)?;

        if n == 0 {
            println!("Server closed connection");
            break;
        }

        print!("{}", std::str::from_utf8(&buf[..n])?);
    }

    Ok(())
}

fn main() {
    if let Err(error) = run() {
        println!("Error: {error}");
        exit(1);
    }
}
