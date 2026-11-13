package main

import (
	"fmt"
	"golang.org/x/mod/module"
	"golang.org/x/mod/sumdb/dirhash"
	"golang.org/x/mod/zip"
	"os"
)

func main() {
	f, e := os.Create(os.Args[2])
	if e != nil {
		panic(e)
	}
	e = zip.CreateFromDir(f, module.Version{Path: os.Args[3], Version: os.Args[4]}, os.Args[1])
	if e != nil {
		panic(e)
	}
	f.Close()
	h, e := dirhash.HashZip(os.Args[2], dirhash.Hash1)
	if e != nil {
		panic(e)
	}
	fmt.Println(h)
}
