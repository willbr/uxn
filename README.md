# uxn

## ops

|       | Stack   |           |               |
|-------|---------|-----------|---------------|
| 0x00  | brk/lit | Break     | a b c M[PC+1] |
| 0x01  | inc     | Increment | a b c+1       |
| 0x02  | pop     | Pop       | a b           |
| 0x03  | dup     | Duplicate | a b c c       |
| 0x04  | nip     | Nip       | a c           |
| 0x05  | swp     | Swap      | a c b         |
| 0x06  | ovr     | Over      | a b c b       |
| 0x07  | rot     | Rotate    | b c a         |

|       | Logic   |           |                   |
|-------|---------|-----------|-------------------|
| 0x08  | equ     | Equal     | a b?c             |
| 0x09  | neq     | Not Equal | a b!c             |
| 0x0a  | gth     | Greater   | a b\>c            |
| 0x0b  | lth     | Lesser    | a b\<c            |
| 0x0c  | jmp     | Jump      | a b {PC+=c}       |
| 0x0d  | jcn     | JumpCond  | a {(b8)PC+=c}     |
| 0x0e  | jsr     | JumpStash | a b {rs.PC PC+=c} |
| 0x0f  | sth     | Stash     | a b {rs.c}        |

|       | Memory |               |                |
|-------|--------|---------------|----------------|
| 0x10  | ldz    | Load Zeropage | a b M[c8]      |
| 0x11  | stz    | Save Zeropage | a {M[c8]=b}    |
| 0x12  | ldr    | Load Rel      | a b M[PC+c8]   |
| 0x13  | str    | Save Rel      | a {M[PC+c8]=b} |
| 0x14  | lda    | Load Abs      | a b M[c16]     |
| 0x15  | sta    | Save Abs      | a b {M[c16]=b} |
| 0x16  | deo    | Device In     | a b D[c8]      |
| 0x17  | dei    | Device Out    | a {D[c8]=b}    |

|       | Arithmetic |              |                   |
|-------|------------|--------------|-------------------|
| 0x18  | add        | Add          | a b+c             |
| 0x19  | sub        | Subtract     | a b-c             |
| 0x1a  | mul        | Multiply     | a b\*c            |
| 0x1b  | div        | Divide       | a b/c             |
| 0x1c  | and        | And          | a b&c             |
| 0x1d  | ora        | Or           | a b\|c            |
| 0x1e  | eor        | Exclusive Or | a b^c             |
| 0x1f  | sft        | Shift        | a b\>\>c8l\<\<c8h |

## labels

|type  |                |         |size     |
|------|----------------|---------|---------|
|.label|literal address |zero page|one byte |
|;label|literal address |         |one short|
|,label|relative address|         |one byte |
|:label|raw address     |         |one short|

    @label
    &sub-label
    label/sub-label

## modes

- keep
- return
- short

## runes

- % macro-define
- | pad absolute
- $ pad relative
- @ label-define
- & sublabel-define
- ~ include
- # literal hex
- . literal addr zero-page
- , literal addr relative
- ; literal addr absolute
- : raw addr absolute
- ' raw char
- " raw word


