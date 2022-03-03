# ie - Indented Expressions

Syntax = Lisp + Python

I love:
https://dwheeler.com/readable/alternative-s-expressions.html

## example

from

    puts "hi"

    define square(x)
        x * x

    puts square(10)

    puts "bye"


to

    ie/newline
    [puts "hi" ie/newline]
    ie/newline
    [define [ie/neoteric square [ie/infix x]] ie/newline
        [x * x ie/newline]]
    ie/newline
    [puts [ie/neoteric square [ie/infix 10]] ie/newline]
    ie/newline
    [puts "bye"]

or without markers

    [puts "hi"]
    [define [ie/neoteric square [ie/infix x]]
        [x * x]]
    [puts [ie/neoteric square [ie/infix 10]]]
    [puts "bye"]


## reader

    read first char
    is it a breakchar char? " \t\n,;()[]{}'
          space
       \t tab is an error
       \n newline
        , comma
        ; semicolon
        ( infix
        { rpn
        [ prefix

    is it a prefix char?
        " string
        * deref
        & address
        ' quote
        % binary
        $ hexadecimal

    read word
    try promote functions
        - promote to number
        - promote to number from hex
        - promote to number from binary


## prefix chars

pointer methods

    * deref
    & addr
    ' quote
    ! not

## add promotion stage

apply a list of transformations to a token

    *something

becomes:

    deref something
    [deref something]
    deref(something)

## include version numbers

    version 0

## heredocs

    puts <<end
    hello,
    How are you doing today?
    bye!
    end

## regex

create a regex object

    /^haha$/

## stdout interface

one token per line

    token

or

    file:row:col:token

example

    in.txt:1:1:def
    in.txt:1:1:main
    in.txt:1:1:ie/newline
    in.txt:2:1:    
    in.txt:2:5:puts
    in.txt:2:10:"hello"

