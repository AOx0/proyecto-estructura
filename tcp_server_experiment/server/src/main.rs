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


    let mut server = TcpServer::new();
    server.next();
    server.next();
}
