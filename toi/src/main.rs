use clap::Parser;
use std::net::TcpStream;
use std::{
    io::{Read, Write},
    process::exit,
};
use text_io::try_read;
use directories::ProjectDirs;

use rustyline::error::ReadlineError;
use rustyline::Editor;

/// DB Client
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// The ip to connect to
    #[arg(required = false, index = 0, default_value = "localhost")]
    ip: String,
    /// The port to connect to
    #[arg(required = false, index = 1, default_value = "9999")]
    port: String,
    /// Non REPL mode
    #[arg(short, long)]
    non_repl: bool,
}

fn run(args: Args) -> Result<(), Box<dyn std::error::Error>> {
    let Args { ip, port, .. } = args;

    let mut stream = TcpStream::connect(format!("{ip}:{port}"))?;

    loop {
        let mut buf = vec![0; 1024 * 2];
        let msg: String = try_read!("{}\n")?;

        if msg.is_empty() {
            continue;
        }

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

fn run_repl(args: Args) -> Result<(), Box<dyn std::error::Error>> {
    let Args { ip, port, .. } = args;

    let project_dirs = ProjectDirs::from("com", "up", "toi").unwrap();

    let history_path = project_dirs.data_dir().join("history.txt");
    let mut stream = TcpStream::connect(format!("{ip}:{port}"))?;

    let mut rl = Editor::<()>::new()?;
    if rl.load_history(&history_path).is_err() {
        println!("Info: No previous history.");
    }

    loop {
        let mut buf = vec![0; 1024 * 2];
        let readline = rl.readline("$ ");
        match readline {
            Ok(msg) => {

                if msg.to_ascii_lowercase() == "clear history" ||
                  msg.to_ascii_lowercase() == "clear_history" {
                    rl.clear_history();
                    continue;
                }

                if msg.is_empty() {
                    continue;
                }

                if !msg.is_ascii() {
                    println!("Message must be ascii");
                    continue;
                }

                stream.write_all(msg.as_bytes())?;
                rl.add_history_entry(msg.as_str());

                if msg == "exit" || msg == "stop" {
                    break;
                }

                let n = stream.read(&mut buf)?;

                if n == 0 {
                    println!("Server closed connection");
                    break;
                }

                print!("{}", std::str::from_utf8(&buf[..n])?);
            },
            Err(ReadlineError::Interrupted) => {
                println!("CTRL-C");
                break
            },
            Err(ReadlineError::Eof) => {
                println!("CTRL-D");
                break
            },
            Err(err) => {
                println!("Error: {:?}", err);
                break
            }
        }
    }
    rl.save_history(&history_path)?;
    Ok(())
}

fn main() {
    let args = Args::parse();

    let result = if args.non_repl {
        run(args)
    } else {
        run_repl(args)
    };

    if let Err(e) = result {
        println!("Error: {}", e);
        exit(1);
    }
}
