use clap::Parser;
use directories::ProjectDirs;
use std::net::TcpStream;
use std::{
    io::{Read, Write},
    process::exit,
};
use sysinfo::{ProcessExt, System, SystemExt};

use rustyline::error::ReadlineError;
use rustyline::Editor;

pub fn kill_database_process() {
    let mut system = System::new_all();
    system.refresh_all();
    let mut process = None;
    for (_, proc_) in system.processes() {
        if proc_.name() == "proyecto-estructura" {
            process = Some(proc_.clone());
        }
    }

    if let Some(proc_) = process {
        proc_.kill_with(sysinfo::Signal::Term);
    }
}

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
}

fn run_repl(args: Args) -> Result<(), Box<dyn std::error::Error>> {
    let Args { ip, port, .. } = args;
    let project_dirs = ProjectDirs::from("com", "up", "toi").unwrap();
    let history_path = project_dirs.data_dir().join("history.txt");

    let mut rl = Editor::<()>::new()?;
    if rl.load_history(&history_path).is_err() {
        println!(
            "Info: No previous history. Generating at {}",
            history_path.display()
        );
    }

    'main: loop {
        let stream = TcpStream::connect(format!("{ip}:{port}"));
        if let Err(e) = stream {
            println!("Error: {}", e);
            println!("Make sure the server is running");
            break 'main;
        }
        let mut stream = stream.unwrap();

        loop {
            let mut buf = vec![0; 1024 * 2];
            let readline = rl.readline("$ ");
            match readline {
                Ok(msg) => {
                    if msg.to_ascii_lowercase() == "clear history"
                        || msg.to_ascii_lowercase() == "clear_history"
                    {
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

                    let result = stream.write_all(msg.as_bytes());
                    if let Err(e) = result {
                        println!("Error: {}", e);
                        break;
                    }
                    rl.add_history_entry(msg.as_str());

                    if msg == "stop" {
                        kill_database_process();
                        break 'main;
                    }

                    if msg == "exit" || msg == "stop" {
                        break 'main;
                    }

                    let n = stream.read(&mut buf);
                    if let Err(e) = n {
                        println!("Error: {}", e);
                        println!("Trying to reconnect. Input your query again");
                        continue 'main;
                    } else {
                        let n = n.unwrap();
                        if n == 0 {
                            println!("Server closed connection");
                            println!("Trying to reconnect. Input your query again");
                            continue 'main;
                        }

                        let conversion = std::str::from_utf8(&buf[..n]);
                        if let Ok(res) = conversion {
                            print!("{}", res);
                        } else {
                            println!("Error: {}", conversion.unwrap_err());
                        }
                    }
                }
                Err(ReadlineError::Interrupted) => {
                    break 'main;
                }
                Err(ReadlineError::Eof) => {
                    break 'main;
                }
                Err(err) => {
                    println!("Error: {:?}", err);
                    break 'main;
                }
            }
        }
    }
    rl.save_history(&history_path)?;
    Ok(())
}

fn main() {
    let args = Args::parse();
    if let Err(e) = run_repl(args) {
        println!("Error: {}", e);
        exit(1);
    }
}
