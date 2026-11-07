# PUN
Esolang where letters are ignored by the compiler

**Syntax:**
```
:: ident var dec
:: ident :: label dec
:: functionname (arg1, arg2) <: :> function dec
& value
= assign
== compare
< less
> greater
-> return
[init, if, step] for loop
? if
! else
<: :> block
\ident jump to label
./ print to stdout
```

**Example:**
```
prints the alphabet
[::0=&65,0<&91,0=0+&1]<:./0:>
```

**Dependencies:**
- gcc
