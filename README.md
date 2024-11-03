# MKit
An example of a simple file hiding kernel module for (modern 6.8) linux.
This idea was inspired by the [adore-ng](https://github.com/yaoyumeng/adore-ng) rootkit. However, adore-ng is 
very outdated (10 years ago) and supported only versions. It's description reads: "Linux rootkit adapted for 2.6 and 3.x". Many stuctures have changed since then. I decided to update the file hiding part as a simple side project.

## Basic Flow
By replacing a `struct file` object's `iterate_shared` function to something we can control we can change the reading directory context. See `hooked_filldir`. This function is then called with each file in the directory. If the name matches the name defined as `HIDE_FILE_NAME` then we simply return ENOENT. 

## Building
1. `sudo apt-get install linux-headers-$(uname -r)`
2. `make`

## Running
1. `sudo insmod mkit.ko`
2. "HIDE_FILE_NAME" is now hidden from usermode programs.
