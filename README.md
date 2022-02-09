# ie - Indented Expressions

Syntax = Lisp + Python

I love:
https://dwheeler.com/readable/alternative-s-expressions.html

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

    row,col,token

example

    1,1,def
    1,1,main
    1,1,ie/newline
    2,1,    
    2,5,puts
    2,10,"hello"

