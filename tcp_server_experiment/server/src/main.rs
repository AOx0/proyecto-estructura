use std::ffi::CString;
use server::*;
use std::io::stdin;

fn main() {
    let mut server = start();

    unsafe {
        for i in 1..5 {
            let s = receive(&mut server);
            communicate(&mut server, s, CString::new("Hola desde el server").unwrap().into_raw());
        }
    }
    stdin().read_line(&mut String::new()).unwrap();

    stop(server);
}
