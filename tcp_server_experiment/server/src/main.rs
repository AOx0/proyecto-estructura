use std::io::stdin;
use server::*;

fn main() {
  let mut server = TcpServer::new();
  server.run_server();
  let mut a: String = String::new();
  stdin().read_line(&mut a).expect("TODO: panic message");
}

