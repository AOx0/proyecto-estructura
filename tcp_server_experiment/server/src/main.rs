use std::io::stdin;
use server::*;

fn main() {
  let server = server::start();

  stdin().read_line(&mut String::new()).unwrap();

  stop(server);
}

