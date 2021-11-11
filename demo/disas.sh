#!/bin/bash


usage() {
  echo "Usage $1 <file> <function>" >&2
  exit 1
}

case $# in
  2);;
  *) usage $0;;
esac

gdb -batch -ex "file $1" -ex "disassemble $2"
