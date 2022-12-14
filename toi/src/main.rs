use clap::{CommandFactory, Parser, Subcommand};
use clap_complete::{generate, Shell};
use directories::ProjectDirs;
use rustyline::error::ReadlineError;
use std::env::set_current_dir;
use std::net::TcpStream;
use std::process::{Command, Stdio};
use std::{
    io::{Read, Write},
    process::exit,
};
use sysinfo::{ProcessExt, Signal, System, SystemExt};
mod dep;

use std::collections::HashSet;

use rustyline::hint::{Hint, Hinter};
use rustyline::Context;
use rustyline::{Cmd, Editor, KeyEvent};
use rustyline_derive::{Completer, Helper, Highlighter, Validator};

#[derive(Completer, Helper, Validator, Highlighter)]
struct DIYHinter {
    // It's simple example of rustyline, for more efficient, please use ** radix trie **
    hints: HashSet<CommandHint>,
}

#[derive(Hash, Debug, PartialEq, Eq)]
struct CommandHint {
    display: String,
    complete_up_to: usize,
}

impl Hint for CommandHint {
    fn display(&self) -> &str {
        &self.display
    }

    fn completion(&self) -> Option<&str> {
        if self.complete_up_to > 0 {
            Some(&self.display[..self.complete_up_to])
        } else {
            None
        }
    }
}

impl CommandHint {
    fn new(text: &str, complete_up_to: &str) -> CommandHint {
        assert!(text.starts_with(complete_up_to));
        CommandHint {
            display: text.into(),
            complete_up_to: complete_up_to.len(),
        }
    }

    fn suffix(&self, strip_chars: usize) -> CommandHint {
        CommandHint {
            display: self.display[strip_chars..].to_owned(),
            complete_up_to: self.complete_up_to.saturating_sub(strip_chars),
        }
    }
}

impl Hinter for DIYHinter {
    type Hint = CommandHint;

    fn hint(&self, line: &str, pos: usize, _ctx: &Context<'_>) -> Option<CommandHint> {
        if line.is_empty() || pos < line.len() {
            return None;
        }

        self.hints.iter().find_map(|hint| {
            // expect hint after word complete, like redis cli, add condition:
            //line.ends_with(" ");
            if hint.display.starts_with(line)
            /*&& line.ends_with(" ")*/
            {
                Some(hint.suffix(pos))
            } else {
                None
            }
        })
    }
}

fn diy_hints() -> HashSet<CommandHint> {
    let mut set = HashSet::new();
    set.insert(CommandHint::new("exit", "exit"));
    set.insert(CommandHint::new("stop", "stop"));
    set.insert(CommandHint::new("SHOW DATABASES;", "SHOW DATABASES;"));
    set.insert(CommandHint::new(
        "SHOW DATABASE database;",
        "SHOW DATABASE ",
    ));
    set.insert(CommandHint::new(
        "SHOW TABLE database.table;",
        "SHOW TABLE ",
    ));
    set.insert(CommandHint::new(
        "SHOW COLUMN database table.column;",
        "SHOW COLUMN ",
    ));
    set.insert(CommandHint::new("SHOW FROM database.table;", "SHOW FROM "));
    set.insert(CommandHint::new(
        "SHOW FROM database table.column;",
        "SHOW FROM ",
    ));
    set
}

pub fn process_exists() -> bool {
    let mut system = System::new_all();
    system.refresh_all();
    for (_, proc_) in system.processes() {
        if proc_.name() == "toidb" {
            return true;
        }
    }

    return false;
}

pub fn kill_database_process(force: bool) {
    let mut system = System::new_all();
    system.refresh_all();
    let mut process = None;
    for (_, proc_) in system.processes() {
        if proc_.name() == "toidb" {
            process = Some(proc_.clone());
        }
    }

    if let Some(proc_) = process {
        if force {
            proc_.kill();
        } else {
            #[cfg(any(target_os = "macos", target_os = "linux"))]
            proc_.kill_with(Signal::Term);

            #[cfg(any(target_os = "windows"))]
            proc_.kill();
        }
    }
}

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct App {
    #[clap(subcommand)]
    pub(crate) command: Commands,
}

#[derive(Subcommand, Debug)]
enum Commands {
    #[clap(about = "Run toidb in the background")]
    Start {
        /// The port to bind to
        #[arg(short, long, default_value = "9999")]
        port: String,
        /// Show server output
        #[arg(short, long)]
        attach: bool,
    },
    #[clap(about = "Stop toidb process")]
    Stop {
        /// Force stop instead of attepmting to smootly end toidb.
        #[arg(short, long)]
        force: bool,
    },
    #[clap(about = "Rerun toidb")]
    Restart {
        /// Force stop instead of attepmting to smootly end toidb.
        #[arg(short, long)]
        force: bool,
        /// The port to bind to
        #[arg(short, long, default_value = "9999")]
        port: String,
        /// Show server output
        #[arg(short, long)]
        attach: bool,
    },
    #[clap(about = "Connect to a toidb instance")]
    Connect {
        /// The ip to connect to
        #[arg(short, long, default_value = "localhost")]
        ip: String,
        /// The port to connect to
        #[arg(short, long, default_value = "9999")]
        port: String,
    },
    #[clap(about = "Generate toi completions for the specified shell")]
    GenerateCompletion {
        #[clap(value_parser(["bash", "fish", "zsh", "powershell", "elvish"]))]
        shell: String,
    },
}

fn run_repl(ip: &str, port: &str) -> Result<(), Box<dyn std::error::Error>> {
    let project_dirs = ProjectDirs::from("com", "up", "toi").unwrap();
    let history_path = project_dirs.data_dir().join("history.txt");
    let h = DIYHinter { hints: diy_hints() };

    let mut rl = Editor::<DIYHinter>::new()?;
    rl.bind_sequence(KeyEvent::from('\t'), Cmd::CompleteHint);
    rl.set_helper(Some(h));
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
                        kill_database_process(false);
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
    let args = App::parse();

    match args.command {
        Commands::Restart { force, attach, port } => {
            if !process_exists() {
                println!("Error: toidb is not running");
            } else {
                kill_database_process(force);
                println!("Stopped toidb");
            }

            if let Err(e) = dep::colocar_dependencias() {
                println!("Error: {e}");
                exit(1);
            }

            let bin_dir = match dep::get_data_dir() {
                Err(e) => {
                    println!("Error al botener la ruta al directorio 'bin'\nError {e}");
                    exit(1);
                }
                Ok(value) => value.join("bin"),
            };

            if let Err(e) = set_current_dir(&bin_dir) {
                println!("Error: {e}");
                exit(1);
            }

            if process_exists() {
                println!("Error: toidb is already running");
                exit(1);
            }

            if !attach {
                Command::new(&bin_dir.join("toidb"))
                    .args(["-p", &port])
                    .stdin(Stdio::null())
                    .stdout(Stdio::null())
                    .stderr(Stdio::null())
                    .spawn()
                    .unwrap();
                println!("Spawned a toidb instance");
            } else {
                Command::new(&bin_dir.join("toidb"))
                    .args(["-p", &port])
                    .spawn()
                    .unwrap()
                    .wait_with_output()
                    .unwrap();
            }
        }
        Commands::Stop { force } => {
            if !process_exists() {
                println!("Error: toidb is not running");
            } else {
                kill_database_process(force);
                println!("Stopped toidb");
            }
        }
        Commands::Connect { ip, port } => {
            if let Err(e) = run_repl(&ip, &port) {
                println!("Error: {}", e);
                exit(1);
            }
        }
        Commands::Start { port, attach } => {
            if let Err(e) = dep::colocar_dependencias() {
                println!("Error: {e}");
                exit(1);
            }

            let bin_dir = match dep::get_data_dir() {
                Err(e) => {
                    println!("Error al botener la ruta al directorio 'bin'\nError {e}");
                    exit(1);
                }
                Ok(value) => value.join("bin"),
            };

            if let Err(e) = set_current_dir(&bin_dir) {
                println!("Error: {e}");
                exit(1);
            }

            if process_exists() {
                println!("Error: toidb is already running");
                exit(1);
            }

            if !attach {
                Command::new(&bin_dir.join("toidb"))
                    .args(["-p", &port])
                    .stdin(Stdio::null())
                    .stdout(Stdio::null())
                    .stderr(Stdio::null())
                    .spawn()
                    .unwrap();
                println!("Spawned a toidb instance");
            } else {
                Command::new(&bin_dir.join("toidb"))
                    .args(["-p", &port])
                    .spawn()
                    .unwrap()
                    .wait_with_output()
                    .unwrap();
            }
        }
        Commands::GenerateCompletion { shell } => {
            let mut matches = App::command();
            match shell.as_str() {
                "bash" => {
                    generate(Shell::Bash, &mut matches, "toi", &mut std::io::stdout());
                }
                "fish" => {
                    generate(Shell::Fish, &mut matches, "toi", &mut std::io::stdout());
                }
                "zsh" => {
                    generate(Shell::Zsh, &mut matches, "toi", &mut std::io::stdout());
                }
                "powershell" => {
                    generate(
                        Shell::PowerShell,
                        &mut matches,
                        "toi",
                        &mut std::io::stdout(),
                    );
                }
                "elvish" => {
                    generate(Shell::Elvish, &mut matches, "toi", &mut std::io::stdout());
                }
                _ => {
                    eprintln!("Invalid shell");
                    std::process::exit(1);
                }
            }
        }
    }
}
