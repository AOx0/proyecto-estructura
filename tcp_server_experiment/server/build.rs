fn main() {
  cxx_build::bridge("src/lib.rs")
    .file("../main.cpp")
    .flag_if_supported("-std=c++14")
    .compile("cxx-demo");
}