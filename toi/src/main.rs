use clap::Parser;
use std::net::TcpStream;
use std::{
    io::{Read, Write},
    process::exit,
};
use text_io::*;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[command(author, version, about = "DB Client for Toi", long_about = None)]
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

    let mut msg: String;
    let mut buf;
    loop {
        buf = vec![0; 512 * 4];
        msg = try_read!("{}\n")?;

        if !msg.is_ascii() {
            println!("Message must be ascii");
            continue;
        }

        let _ = stream.write(msg.as_bytes())?;

        if msg == "exit" || msg == "stop" {
            break;
        }

        let _ = stream.read(&mut buf)?;

        let response = String::from_utf8(buf.to_vec())?;

        print!("{response}");
    }

    Ok(())
}

fn main() {
    if let Err(error) = run() {
        println!("Error: {error}");
        exit(1);
    }
}
