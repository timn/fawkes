package main

import "C"
import "fmt"
import "text/template"
import "os"


type Inventory struct {
	Material string
	Count    uint
}

func main() {}

//export Func
func Func() {
  fmt.Println("hello world")
  tmpl, err := template.New("test").Parse("{{.Count}} items are made of {{.Material}}\n")
  if err != nil { panic(err) }
  sweaters := Inventory{Material: "wool", Count: 17}
  err = tmpl.Execute(os.Stdout, sweaters)
  if err != nil { panic(err) }
}

