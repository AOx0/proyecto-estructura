use server::*;
use std::ffi::CString;
use std::io::stdin;

fn main() {
    let mut server = start();

    stdin().read_line(&mut String::new()).unwrap();

    stop(server);
}
