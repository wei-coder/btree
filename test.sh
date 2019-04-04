#!/usr/bin/expect

set timeout 2

spawn ./btree

expect  -re "\]*" { send "p\r" }
expect -re "\]*" { send "q\r" }
expect eof
