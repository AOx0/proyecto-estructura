use std::{ffi::CString, os::raw::c_char};

use server::*;

fn main() {
    /*unsafe {
        println!("Starting server: ");
        let server = start_server();

        let mut buf = String::new();
        println!("Input anything to end: ");
        std::io::stdin().read_line(&mut buf).unwrap();
        println!("Ending server: ");
        end_server(server);
        println!("Finished");
    }*/

    unsafe {
        let server = start_server();

        for i in 0..5 {
            let input = read(server);
            let input_str = CString::from_raw(input as *mut i8);
            let input_str = input_str.to_str().unwrap();

            println!("{}", input_str);

            match input_str {
                "Hola" => {
                    write(server, CString::from_vec_unchecked("Hola".as_bytes().to_vec()).as_ptr() as *mut c_char)
                },
                _ => {
                    write(server, CString::from_vec_unchecked(b"Unknown command".to_vec()).as_ptr() as *mut c_char );
                }
            }

        }

        end_server(server);


    }

}

