/*
local i: integer, k: integer, prime: integer, count: integer
local flags: integer[] = table.intarray(8190)

for iter=0,100000  do
    count = 0
    for i=0,8190 do
        flags[i] = 1
    end
    for i=0,8190 do
        if flags[i] == 1 then
            prime = i + i + 3;
            for k = i + prime, 8190, prime do
                flags[k] = 0
            end
            count = count + 1
        end
    end
end
return count
function()
--upvalues  _ENV*
--[local symbols] i, k, prime, count, flags
  local
  --[symbols]
    i --local symbol integer 
   ,
    k --local symbol integer 
   ,
    prime --local symbol integer 
   ,
    count --local symbol integer 
  local
  --[symbols]
    flags --local symbol integer[] 
  --[expressions]
    --[suffixed expr start] any
     --[primary start] any
       table --global symbol any 
     --[primary end]
     --[suffix list start]
       --[field selector start] any
        .
         'intarray'
       --[field selector end]
       --[function call start] any
        (
          8190
        )
       --[function call end]
     --[suffix list end]
    --[suffixed expr end]
  for
  --[local symbols] iter
    iter --local symbol any 
  =
    0
   ,
    100000
  do
     --[expression statement start]
      --[var list start]
        --[suffixed expr start] integer
         --[primary start] integer
           count --local symbol integer 
         --[primary end]
        --[suffixed expr end]
      = --[var list end]
      --[expression list start]
        0
      --[expression list end]
     --[expression statement end]
     for
     --[local symbols] i
       i --local symbol any 
     =
       0
      ,
       8190
     do
        --[expression statement start]
         --[var list start]
           --[suffixed expr start] any
            --[primary start] integer[]
              flags --local symbol integer[] 
            --[primary end]
            --[suffix list start]
              --[Y index start] any
               [
                --[suffixed expr start] any
                 --[primary start] any
                   i --local symbol any 
                 --[primary end]
                --[suffixed expr end]
               ]
              --[Y index end]
            --[suffix list end]
           --[suffixed expr end]
         = --[var list end]
         --[expression list start]
           1
         --[expression list end]
        --[expression statement end]
     end
     for
     --[local symbols] i
       i --local symbol any 
     =
       0
      ,
       8190
     do
        if
         --[binary expr start] any
          --[suffixed expr start] any
           --[primary start] integer[]
             flags --local symbol integer[] 
           --[primary end]
           --[suffix list start]
             --[Y index start] any
              [
               --[suffixed expr start] any
                --[primary start] any
                  i --local symbol any 
                --[primary end]
               --[suffixed expr end]
              ]
             --[Y index end]
           --[suffix list end]
          --[suffixed expr end]
         ==
          1
         --[binary expr end]
        then
          --[expression statement start]
           --[var list start]
             --[suffixed expr start] integer
              --[primary start] integer
                prime --local symbol integer 
              --[primary end]
             --[suffixed expr end]
           = --[var list end]
           --[expression list start]
             --[binary expr start] any
              --[binary expr start] any
               --[suffixed expr start] any
                --[primary start] any
                  i --local symbol any 
                --[primary end]
               --[suffixed expr end]
              +
               --[suffixed expr start] any
                --[primary start] any
                  i --local symbol any 
                --[primary end]
               --[suffixed expr end]
              --[binary expr end]
             +
              3
             --[binary expr end]
           --[expression list end]
          --[expression statement end]
          for
          --[local symbols] k
            k --local symbol any 
          =
            --[binary expr start] any
             --[suffixed expr start] any
              --[primary start] any
                i --local symbol any 
              --[primary end]
             --[suffixed expr end]
            +
             --[suffixed expr start] integer
              --[primary start] integer
                prime --local symbol integer 
              --[primary end]
             --[suffixed expr end]
            --[binary expr end]
           ,
            8190
           ,
            --[suffixed expr start] integer
             --[primary start] integer
               prime --local symbol integer 
             --[primary end]
            --[suffixed expr end]
          do
             --[expression statement start]
              --[var list start]
                --[suffixed expr start] any
                 --[primary start] integer[]
                   flags --local symbol integer[] 
                 --[primary end]
                 --[suffix list start]
                   --[Y index start] any
                    [
                     --[suffixed expr start] any
                      --[primary start] any
                        k --local symbol any 
                      --[primary end]
                     --[suffixed expr end]
                    ]
                   --[Y index end]
                 --[suffix list end]
                --[suffixed expr end]
              = --[var list end]
              --[expression list start]
                0
              --[expression list end]
             --[expression statement end]
          end
          --[expression statement start]
           --[var list start]
             --[suffixed expr start] integer
              --[primary start] integer
                count --local symbol integer 
              --[primary end]
             --[suffixed expr end]
           = --[var list end]
           --[expression list start]
             --[binary expr start] any
              --[suffixed expr start] integer
               --[primary start] integer
                 count --local symbol integer 
               --[primary end]
              --[suffixed expr end]
             +
              1
             --[binary expr end]
           --[expression list end]
          --[expression statement end]
        end
     end
  end
  return
    --[suffixed expr start] integer
     --[primary start] integer
       count --local symbol integer 
     --[primary end]
    --[suffixed expr end]
end
function()
--upvalues  _ENV*
--[local symbols] i, k, prime, count, flags
  local
  --[symbols]
    i --local symbol integer 
   ,
    k --local symbol integer 
   ,
    prime --local symbol integer 
   ,
    count --local symbol integer 
  local
  --[symbols]
    flags --local symbol integer[] 
  --[expressions]
    --[suffixed expr start] any
     --[primary start] any
       table --global symbol any 
     --[primary end]
     --[suffix list start]
       --[field selector start] any
        .
         'intarray'
       --[field selector end]
       --[function call start] any
        (
          8190
        )
       --[function call end]
     --[suffix list end]
    --[suffixed expr end]
  for
  --[local symbols] iter
    iter --local symbol integer 
  =
    0
   ,
    100000
  do
     --[expression statement start]
      --[var list start]
        --[suffixed expr start] integer
         --[primary start] integer
           count --local symbol integer 
         --[primary end]
        --[suffixed expr end]
      = --[var list end]
      --[expression list start]
        0
      --[expression list end]
     --[expression statement end]
     for
     --[local symbols] i
       i --local symbol integer 
     =
       0
      ,
       8190
     do
        --[expression statement start]
         --[var list start]
           --[suffixed expr start] integer
            --[primary start] integer[]
              flags --local symbol integer[] 
            --[primary end]
            --[suffix list start]
              --[Y index start] integer
               [
                --[suffixed expr start] integer
                 --[primary start] integer
                   i --local symbol integer 
                 --[primary end]
                --[suffixed expr end]
               ]
              --[Y index end]
            --[suffix list end]
           --[suffixed expr end]
         = --[var list end]
         --[expression list start]
           1
         --[expression list end]
        --[expression statement end]
     end
     for
     --[local symbols] i
       i --local symbol integer 
     =
       0
      ,
       8190
     do
        if
         --[binary expr start] boolean
          --[suffixed expr start] integer
           --[primary start] integer[]
             flags --local symbol integer[] 
           --[primary end]
           --[suffix list start]
             --[Y index start] integer
              [
               --[suffixed expr start] integer
                --[primary start] integer
                  i --local symbol integer 
                --[primary end]
               --[suffixed expr end]
              ]
             --[Y index end]
           --[suffix list end]
          --[suffixed expr end]
         ==
          1
         --[binary expr end]
        then
          --[expression statement start]
           --[var list start]
             --[suffixed expr start] integer
              --[primary start] integer
                prime --local symbol integer 
              --[primary end]
             --[suffixed expr end]
           = --[var list end]
           --[expression list start]
             --[binary expr start] integer
              --[binary expr start] integer
               --[suffixed expr start] integer
                --[primary start] integer
                  i --local symbol integer 
                --[primary end]
               --[suffixed expr end]
              +
               --[suffixed expr start] integer
                --[primary start] integer
                  i --local symbol integer 
                --[primary end]
               --[suffixed expr end]
              --[binary expr end]
             +
              3
             --[binary expr end]
           --[expression list end]
          --[expression statement end]
          for
          --[local symbols] k
            k --local symbol integer 
          =
            --[binary expr start] integer
             --[suffixed expr start] integer
              --[primary start] integer
                i --local symbol integer 
              --[primary end]
             --[suffixed expr end]
            +
             --[suffixed expr start] integer
              --[primary start] integer
                prime --local symbol integer 
              --[primary end]
             --[suffixed expr end]
            --[binary expr end]
           ,
            8190
           ,
            --[suffixed expr start] integer
             --[primary start] integer
               prime --local symbol integer 
             --[primary end]
            --[suffixed expr end]
          do
             --[expression statement start]
              --[var list start]
                --[suffixed expr start] integer
                 --[primary start] integer[]
                   flags --local symbol integer[] 
                 --[primary end]
                 --[suffix list start]
                   --[Y index start] integer
                    [
                     --[suffixed expr start] integer
                      --[primary start] integer
                        k --local symbol integer 
                      --[primary end]
                     --[suffixed expr end]
                    ]
                   --[Y index end]
                 --[suffix list end]
                --[suffixed expr end]
              = --[var list end]
              --[expression list start]
                0
              --[expression list end]
             --[expression statement end]
          end
          --[expression statement start]
           --[var list start]
             --[suffixed expr start] integer
              --[primary start] integer
                count --local symbol integer 
              --[primary end]
             --[suffixed expr end]
           = --[var list end]
           --[expression list start]
             --[binary expr start] integer
              --[suffixed expr start] integer
               --[primary start] integer
                 count --local symbol integer 
               --[primary end]
              --[suffixed expr end]
             +
              1
             --[binary expr end]
           --[expression list end]
          --[expression statement end]
        end
     end
  end
  return
    --[suffixed expr start] integer
     --[primary start] integer
       count --local symbol integer 
     --[primary end]
    --[suffixed expr end]
end
define Proc%1
L0 (entry)
	LOADGLOBAL {Upval(_ENV), 'table' Ks(0)} {T(0)}
	GETsk {T(0), 'intarray' Ks(1)} {T(1)}
	CALL {T(1), 8190 Kint(0)} {T(1..), 1 Kint(1)}
	TOIARRAY {T(1[1..])}
	MOV {T(1[1..])} {local(flags, 0)}
	MOV {0 Kint(2)} {Tint(5)}
	MOV {100000 Kint(3)} {Tint(6)}
	MOV {1 Kint(1)} {Tint(7)}
	SUBii {Tint(5), Tint(7)} {Tint(5)}
	BR {L2}
L1 (exit)
L2
	ADDii {Tint(5), Tint(7)} {Tint(5)}
	BR {L3}
L3
	LIii {Tint(6), Tint(5)} {Tbool(8)}
	CBR {Tbool(8)} {L5, L4}
L4
	MOV {Tint(5)} {Tint(4)}
	MOVi {0 Kint(2)} {Tint(3)}
	MOV {0 Kint(2)} {Tint(10)}
	MOV {8190 Kint(0)} {Tint(11)}
	MOV {1 Kint(1)} {Tint(12)}
	SUBii {Tint(10), Tint(12)} {Tint(10)}
	BR {L6}
L5
	RET {Tint(3)} {L1}
L6
	ADDii {Tint(10), Tint(12)} {Tint(10)}
	BR {L7}
L7
	LIii {Tint(11), Tint(10)} {Tbool(13)}
	CBR {Tbool(13)} {L9, L8}
L8
	MOV {Tint(10)} {Tint(9)}
	IAPUTiv {1 Kint(1)} {local(flags, 0), Tint(9)}
	BR {L6}
L9
	MOV {0 Kint(2)} {Tint(11)}
	MOV {8190 Kint(0)} {Tint(12)}
	MOV {1 Kint(1)} {Tint(13)}
	SUBii {Tint(11), Tint(13)} {Tint(11)}
	BR {L10}
L10
	ADDii {Tint(11), Tint(13)} {Tint(11)}
	BR {L11}
L11
	LIii {Tint(12), Tint(11)} {Tbool(9)}
	CBR {Tbool(9)} {L13, L12}
L12
	MOV {Tint(11)} {Tint(10)}
	BR {L14}
L13
	BR {L2}
L14
	IAGETik {local(flags, 0), Tint(10)} {Tint(14)}
	EQii {Tint(14), 1 Kint(1)} {Tbool(15)}
	CBR {Tbool(15)} {L15, L16}
L15
	ADDii {Tint(10), Tint(10)} {Tint(14)}
	ADDii {Tint(14), 3 Kint(4)} {Tint(16)}
	MOVi {Tint(16)} {Tint(2)}
	ADDii {Tint(10), Tint(2)} {Tint(14)}
	MOV {Tint(14)} {Tint(17)}
	MOV {8190 Kint(0)} {Tint(18)}
	MOV {Tint(2)} {Tint(19)}
	LIii {0 Kint(2), Tint(19)} {Tbool(20)}
	SUBii {Tint(17), Tint(19)} {Tint(17)}
	BR {L17}
L16
	BR {L10}
L17
	ADDii {Tint(17), Tint(19)} {Tint(17)}
	CBR {Tbool(20)} {L18, L19}
L18
	LIii {Tint(18), Tint(17)} {Tbool(21)}
	CBR {Tbool(21)} {L21, L20}
L19
	LIii {Tint(17), Tint(18)} {Tbool(21)}
	CBR {Tbool(21)} {L21, L20}
L20
	MOV {Tint(17)} {Tint(16)}
	IAPUTiv {0 Kint(2)} {local(flags, 0), Tint(16)}
	BR {L17}
L21
	ADDii {Tint(3), 1 Kint(1)} {Tint(17)}
	MOVi {Tint(17)} {Tint(3)}
	BR {L16}
digraph Proc1 {
L0 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L0</B></TD></TR>
<TR><TD>LOADGLOBAL {Upval(_ENV), 'table' Ks(0)} {T(0)}</TD></TR>
<TR><TD>GETsk {T(0), 'intarray' Ks(1)} {T(1)}</TD></TR>
<TR><TD>CALL {T(1), 8190 Kint(0)} {T(1..), 1 Kint(1)}</TD></TR>
<TR><TD>TOIARRAY {T(1[1..])}</TD></TR>
<TR><TD>MOV {T(1[1..])} {local(flags, 0)}</TD></TR>
<TR><TD>MOV {0 Kint(2)} {Tint(5)}</TD></TR>
<TR><TD>MOV {100000 Kint(3)} {Tint(6)}</TD></TR>
<TR><TD>MOV {1 Kint(1)} {Tint(7)}</TD></TR>
<TR><TD>SUBii {Tint(5), Tint(7)} {Tint(5)}</TD></TR>
<TR><TD>BR {L2}</TD></TR>
</TABLE>>];
L0 -> L2
L2 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L2</B></TD></TR>
<TR><TD>ADDii {Tint(5), Tint(7)} {Tint(5)}</TD></TR>
<TR><TD>BR {L3}</TD></TR>
</TABLE>>];
L2 -> L3
L3 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L3</B></TD></TR>
<TR><TD>LIii {Tint(6), Tint(5)} {Tbool(8)}</TD></TR>
<TR><TD>CBR {Tbool(8)} {L5, L4}</TD></TR>
</TABLE>>];
L3 -> L5
L3 -> L4
L4 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L4</B></TD></TR>
<TR><TD>MOV {Tint(5)} {Tint(4)}</TD></TR>
<TR><TD>MOVi {0 Kint(2)} {Tint(3)}</TD></TR>
<TR><TD>MOV {0 Kint(2)} {Tint(10)}</TD></TR>
<TR><TD>MOV {8190 Kint(0)} {Tint(11)}</TD></TR>
<TR><TD>MOV {1 Kint(1)} {Tint(12)}</TD></TR>
<TR><TD>SUBii {Tint(10), Tint(12)} {Tint(10)}</TD></TR>
<TR><TD>BR {L6}</TD></TR>
</TABLE>>];
L4 -> L6
L5 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L5</B></TD></TR>
<TR><TD>RET {Tint(3)} {L1}</TD></TR>
</TABLE>>];
L5 -> L1
L6 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L6</B></TD></TR>
<TR><TD>ADDii {Tint(10), Tint(12)} {Tint(10)}</TD></TR>
<TR><TD>BR {L7}</TD></TR>
</TABLE>>];
L6 -> L7
L7 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L7</B></TD></TR>
<TR><TD>LIii {Tint(11), Tint(10)} {Tbool(13)}</TD></TR>
<TR><TD>CBR {Tbool(13)} {L9, L8}</TD></TR>
</TABLE>>];
L7 -> L9
L7 -> L8
L8 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L8</B></TD></TR>
<TR><TD>MOV {Tint(10)} {Tint(9)}</TD></TR>
<TR><TD>IAPUTiv {1 Kint(1)} {local(flags, 0), Tint(9)}</TD></TR>
<TR><TD>BR {L6}</TD></TR>
</TABLE>>];
L8 -> L6
L9 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L9</B></TD></TR>
<TR><TD>MOV {0 Kint(2)} {Tint(11)}</TD></TR>
<TR><TD>MOV {8190 Kint(0)} {Tint(12)}</TD></TR>
<TR><TD>MOV {1 Kint(1)} {Tint(13)}</TD></TR>
<TR><TD>SUBii {Tint(11), Tint(13)} {Tint(11)}</TD></TR>
<TR><TD>BR {L10}</TD></TR>
</TABLE>>];
L9 -> L10
L10 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L10</B></TD></TR>
<TR><TD>ADDii {Tint(11), Tint(13)} {Tint(11)}</TD></TR>
<TR><TD>BR {L11}</TD></TR>
</TABLE>>];
L10 -> L11
L11 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L11</B></TD></TR>
<TR><TD>LIii {Tint(12), Tint(11)} {Tbool(9)}</TD></TR>
<TR><TD>CBR {Tbool(9)} {L13, L12}</TD></TR>
</TABLE>>];
L11 -> L13
L11 -> L12
L12 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L12</B></TD></TR>
<TR><TD>MOV {Tint(11)} {Tint(10)}</TD></TR>
<TR><TD>BR {L14}</TD></TR>
</TABLE>>];
L12 -> L14
L13 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L13</B></TD></TR>
<TR><TD>BR {L2}</TD></TR>
</TABLE>>];
L13 -> L2
L14 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L14</B></TD></TR>
<TR><TD>IAGETik {local(flags, 0), Tint(10)} {Tint(14)}</TD></TR>
<TR><TD>EQii {Tint(14), 1 Kint(1)} {Tbool(15)}</TD></TR>
<TR><TD>CBR {Tbool(15)} {L15, L16}</TD></TR>
</TABLE>>];
L14 -> L15
L14 -> L16
L15 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L15</B></TD></TR>
<TR><TD>ADDii {Tint(10), Tint(10)} {Tint(14)}</TD></TR>
<TR><TD>ADDii {Tint(14), 3 Kint(4)} {Tint(16)}</TD></TR>
<TR><TD>MOVi {Tint(16)} {Tint(2)}</TD></TR>
<TR><TD>ADDii {Tint(10), Tint(2)} {Tint(14)}</TD></TR>
<TR><TD>MOV {Tint(14)} {Tint(17)}</TD></TR>
<TR><TD>MOV {8190 Kint(0)} {Tint(18)}</TD></TR>
<TR><TD>MOV {Tint(2)} {Tint(19)}</TD></TR>
<TR><TD>LIii {0 Kint(2), Tint(19)} {Tbool(20)}</TD></TR>
<TR><TD>SUBii {Tint(17), Tint(19)} {Tint(17)}</TD></TR>
<TR><TD>BR {L17}</TD></TR>
</TABLE>>];
L15 -> L17
L16 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L16</B></TD></TR>
<TR><TD>BR {L10}</TD></TR>
</TABLE>>];
L16 -> L10
L17 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L17</B></TD></TR>
<TR><TD>ADDii {Tint(17), Tint(19)} {Tint(17)}</TD></TR>
<TR><TD>CBR {Tbool(20)} {L18, L19}</TD></TR>
</TABLE>>];
L17 -> L18
L17 -> L19
L18 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L18</B></TD></TR>
<TR><TD>LIii {Tint(18), Tint(17)} {Tbool(21)}</TD></TR>
<TR><TD>CBR {Tbool(21)} {L21, L20}</TD></TR>
</TABLE>>];
L18 -> L21
L18 -> L20
L19 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L19</B></TD></TR>
<TR><TD>LIii {Tint(17), Tint(18)} {Tbool(21)}</TD></TR>
<TR><TD>CBR {Tbool(21)} {L21, L20}</TD></TR>
</TABLE>>];
L19 -> L21
L19 -> L20
L20 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L20</B></TD></TR>
<TR><TD>MOV {Tint(17)} {Tint(16)}</TD></TR>
<TR><TD>IAPUTiv {0 Kint(2)} {local(flags, 0), Tint(16)}</TD></TR>
<TR><TD>BR {L17}</TD></TR>
</TABLE>>];
L20 -> L17
L21 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L21</B></TD></TR>
<TR><TD>ADDii {Tint(3), 1 Kint(1)} {Tint(17)}</TD></TR>
<TR><TD>MOVi {Tint(17)} {Tint(3)}</TD></TR>
<TR><TD>BR {L16}</TD></TR>
</TABLE>>];
L21 -> L16
}

*/
#ifdef __MIRC__
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT16_TYPE__ int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT8_TYPE__ int8_t;
typedef __UINT8_TYPE__ uint8_t;
#define NULL ((void *)0)
#define EXPORT
#else
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
#endif
typedef size_t lu_mem;
typedef unsigned char lu_byte;
typedef uint16_t LuaType;
typedef struct lua_State lua_State;
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8
#define LUA_OK  0
typedef enum {TM_INDEX,TM_NEWINDEX,TM_GC,
	TM_MODE,TM_LEN,TM_EQ,TM_ADD,TM_SUB,TM_MUL,
	TM_MOD,TM_POW,TM_DIV,TM_IDIV,TM_BAND,TM_BOR,
	TM_BXOR,TM_SHL,TM_SHR,TM_UNM,TM_BNOT,TM_LT,
	TM_LE,TM_CONCAT,TM_CALL,TM_N
} TMS;
typedef double lua_Number;
typedef int64_t lua_Integer;
typedef uint64_t lua_Unsigned;
typedef int (*lua_CFunction) (lua_State *L);
typedef union {
	lua_Number n;
	double u;
	void *s;
	lua_Integer i;
	long l;
} L_Umaxalign;
#define lua_assert(c)		((void)0)
#define check_exp(c,e)		(e)
#define lua_longassert(c)	((void)0)
#define luai_apicheck(l,e)	lua_assert(e)
#define api_check(l,e,msg)	luai_apicheck(l,(e) && msg)
#define UNUSED(x)	((void)(x))
#define cast(t, exp)	((t)(exp))
#define cast_void(i)	cast(void, (i))
#define cast_byte(i)	cast(lu_byte, (i))
#define cast_num(i)	cast(lua_Number, (i))
#define cast_int(i)	cast(int, (i))
#define cast_uchar(i)	cast(unsigned char, (i))
#define l_castS2U(i)	((lua_Unsigned)(i))
#define l_castU2S(i)	((lua_Integer)(i))
#define l_noret		void
typedef unsigned int Instruction;
#define luai_numidiv(L,a,b)     ((void)L, l_floor(luai_numdiv(L,a,b)))
#define luai_numdiv(L,a,b)      ((a)/(b))
#define luai_nummod(L,a,b,m)  \
  { (m) = l_mathop(fmod)(a,b); if ((m)*(b) < 0) (m) += (b); }
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))
#define RAVI_TFCF	(LUA_TFUNCTION | (4 << 4))
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))
#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))
#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))
#define RAVI_TIARRAY (LUA_TTABLE | (1 << 4))
#define RAVI_TFARRAY (LUA_TTABLE | (2 << 4))
#define BIT_ISCOLLECTABLE	(1 << 15)
#define ctb(t)			((t) | BIT_ISCOLLECTABLE)
typedef struct GCObject GCObject;
#define CommonHeader	GCObject *next; lu_byte tt; lu_byte marked
struct GCObject {
  CommonHeader;
};
typedef union Value {
  GCObject *gc;
  void *p;
  int b;
  lua_CFunction f;
  lua_Integer i;
  lua_Number n;
} Value;
#define TValuefields	Value value_; LuaType tt_
typedef struct lua_TValue {
  TValuefields;
} TValue;
#define NILCONSTANT	{NULL}, LUA_TNIL
#define val_(o)		((o)->value_)
#define rttype(o)	((o)->tt_)
#define novariant(x)	((x) & 0x0F)
#define ttype(o)	(rttype(o) & 0x7F)
#define ttnov(o)	(novariant(rttype(o)))
#define checktag(o,t)		(rttype(o) == (t))
#define checktype(o,t)		(ttnov(o) == (t))
#define ttisnumber(o)		checktype((o), LUA_TNUMBER)
#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)
#define ttisinteger(o)		checktag((o), LUA_TNUMINT)
#define ttisnil(o)		checktag((o), LUA_TNIL)
#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)
#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)
#define ttisstring(o)		checktype((o), LUA_TSTRING)
#define ttisshrstring(o)	checktag((o), ctb(LUA_TSHRSTR))
#define ttislngstring(o)	checktag((o), ctb(LUA_TLNGSTR))
#define ttistable(o)		checktype((o), LUA_TTABLE)
#define ttisiarray(o)    checktag((o), ctb(RAVI_TIARRAY))
#define ttisfarray(o)    checktag((o), ctb(RAVI_TFARRAY))
#define ttisarray(o)     (ttisiarray(o) || ttisfarray(o))
#define ttisLtable(o)    checktag((o), ctb(LUA_TTABLE))
#define ttisfunction(o)		checktype(o, LUA_TFUNCTION)
#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)
#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))
#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))
#define ttislcf(o)		checktag((o), LUA_TLCF)
#define ttisfcf(o) (ttype(o) == RAVI_TFCF)
#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))
#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))
#define ttisdeadkey(o)		checktag((o), LUA_TDEADKEY)
#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)
#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)
#define nvalue(o)	check_exp(ttisnumber(o), \
	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
#define gcvalue(o)	check_exp(iscollectable(o), val_(o).gc)
#define pvalue(o)	check_exp(ttislightuserdata(o), val_(o).p)
#define tsvalue(o)	check_exp(ttisstring(o), gco2ts(val_(o).gc))
#define uvalue(o)	check_exp(ttisfulluserdata(o), gco2u(val_(o).gc))
#define clvalue(o)	check_exp(ttisclosure(o), gco2cl(val_(o).gc))
#define clLvalue(o)	check_exp(ttisLclosure(o), gco2lcl(val_(o).gc))
#define clCvalue(o)	check_exp(ttisCclosure(o), gco2ccl(val_(o).gc))
#define fvalue(o)	check_exp(ttislcf(o), val_(o).f)
#define fcfvalue(o) check_exp(ttisfcf(o), val_(o).p)
#define hvalue(o)	check_exp(ttistable(o), gco2t(val_(o).gc))
#define arrvalue(o) check_exp(ttisarray(o), gco2array(val_(o).gc))
#define arrvalue(o) check_exp(ttisarray(o), gco2array(val_(o).gc))
#define bvalue(o)	check_exp(ttisboolean(o), val_(o).b)
#define thvalue(o)	check_exp(ttisthread(o), gco2th(val_(o).gc))
#define deadvalue(o)	check_exp(ttisdeadkey(o), cast(void *, val_(o).gc))
#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))
#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)
#define righttt(obj)		(ttype(obj) == gcvalue(obj)->tt)
#define checkliveness(L,obj) \
	lua_longassert(!iscollectable(obj) || \
		(righttt(obj) && (L == NULL || !isdead(G(L),gcvalue(obj)))))
#define settt_(o,t)	((o)->tt_=(t))
#define setfltvalue(obj,x) \
  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }
#define chgfltvalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x); }
#define setivalue(obj,x) \
  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }
#define chgivalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x); }
#define setnilvalue(obj) settt_(obj, LUA_TNIL)
#define setfvalue(obj,x) \
  { TValue *io=(obj); val_(io).f=(x); settt_(io, LUA_TLCF); }
#define setfvalue_fastcall(obj, x, tag) \
{ \
    TValue *io = (obj);   \
    lua_assert(tag >= 1 && tag < 0x80); \
    val_(io).p = (x);     \
    settt_(io, ((tag << 8) | RAVI_TFCF)); \
}
#define setpvalue(obj,x) \
  { TValue *io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA); }
#define setbvalue(obj,x) \
  { TValue *io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN); }
#define setgcovalue(L,obj,x) \
  { TValue *io = (obj); GCObject *i_g=(x); \
    val_(io).gc = i_g; settt_(io, ctb(i_g->tt)); }
#define setsvalue(L,obj,x) \
  { TValue *io = (obj); TString *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(x_->tt)); \
    checkliveness(L,io); }
#define setuvalue(L,obj,x) \
  { TValue *io = (obj); Udata *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TUSERDATA)); \
    checkliveness(L,io); }
#define setthvalue(L,obj,x) \
  { TValue *io = (obj); lua_State *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTHREAD)); \
    checkliveness(L,io); }
#define setclLvalue(L,obj,x) \
  { TValue *io = (obj); LClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TLCL)); \
    checkliveness(L,io); }
#define setclCvalue(L,obj,x) \
  { TValue *io = (obj); CClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TCCL)); \
    checkliveness(L,io); }
#define sethvalue(L,obj,x) \
  { TValue *io = (obj); Table *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTABLE)); \
    checkliveness(L,io); }
#define setiarrayvalue(L,obj,x) \
  { TValue *io = (obj); Table *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(RAVI_TIARRAY)); \
    checkliveness(L,io); }
#define setfarrayvalue(L,obj,x) \
  { TValue *io = (obj); Table *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(RAVI_TFARRAY)); \
    checkliveness(L,io); }
#define setdeadvalue(obj)	settt_(obj, LUA_TDEADKEY)
#define setobj(L,obj1,obj2) \
	{ TValue *io1=(obj1); const TValue *io2=(obj2); io1->tt_ = io2->tt_; val_(io1).n = val_(io2).n; \
	  (void)L; checkliveness(L,io1); }
#define setobjs2s	setobj
#define setobj2s	setobj
#define setsvalue2s	setsvalue
#define sethvalue2s	sethvalue
#define setptvalue2s	setptvalue
#define setobjt2t	setobj
#define setobj2n	setobj
#define setsvalue2n	setsvalue
#define setobj2t	setobj
typedef TValue *StkId;
typedef struct TString {
	CommonHeader;
	lu_byte extra;
	lu_byte shrlen;
	unsigned int hash;
	union {
		size_t lnglen;
		struct TString *hnext;
	} u;
} TString;
typedef union UTString {
	L_Umaxalign dummy;
	TString tsv;
} UTString;
#define getstr(ts)  \
  check_exp(sizeof((ts)->extra), cast(char *, (ts)) + sizeof(UTString))
#define svalue(o)       getstr(tsvalue(o))
#define tsslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)
#define vslen(o)	tsslen(tsvalue(o))
typedef struct Udata {
	CommonHeader;
	LuaType ttuv_;
	struct Table *metatable;
	size_t len;
	union Value user_;
} Udata;
typedef union UUdata {
	L_Umaxalign dummy;
	Udata uv;
} UUdata;
#define getudatamem(u)  \
  check_exp(sizeof((u)->ttuv_), (cast(char*, (u)) + sizeof(UUdata)))
#define setuservalue(L,u,o) \
	{ const TValue *io=(o); Udata *iu = (u); \
	  iu->user_ = io->value_; iu->ttuv_ = rttype(io); \
	  checkliveness(L,io); }
#define getuservalue(L,u,o) \
	{ TValue *io=(o); const Udata *iu = (u); \
	  io->value_ = iu->user_; settt_(io, iu->ttuv_); \
	  checkliveness(L,io); }
typedef enum {
RAVI_TI_NIL,
RAVI_TI_FALSE,
RAVI_TI_TRUE,
RAVI_TI_INTEGER,
RAVI_TI_FLOAT,
RAVI_TI_INTEGER_ARRAY,
RAVI_TI_FLOAT_ARRAY,
RAVI_TI_TABLE,
RAVI_TI_STRING,
RAVI_TI_FUNCTION,
RAVI_TI_USERDATA,
RAVI_TI_OTHER
} ravi_type_index;
typedef uint32_t ravi_type_map;
#define RAVI_TM_NIL (((ravi_type_map)1)<<RAVI_TI_NIL)
#define RAVI_TM_FALSE (((ravi_type_map)1)<<RAVI_TI_FALSE)
#define RAVI_TM_TRUE (((ravi_type_map)1)<<RAVI_TI_TRUE)
#define RAVI_TM_INTEGER (((ravi_type_map)1)<<RAVI_TI_INTEGER)
#define RAVI_TM_FLOAT (((ravi_type_map)1)<<RAVI_TI_FLOAT)
#define RAVI_TM_INTEGER_ARRAY (((ravi_type_map)1)<<RAVI_TI_INTEGER_ARRAY)
#define RAVI_TM_FLOAT_ARRAY (((ravi_type_map)1)<<RAVI_TI_FLOAT_ARRAY)
#define RAVI_TM_TABLE (((ravi_type_map)1)<<RAVI_TI_TABLE)
#define RAVI_TM_STRING (((ravi_type_map)1)<<RAVI_TI_STRING)
#define RAVI_TM_FUNCTION (((ravi_type_map)1)<<RAVI_TI_FUNCTION)
#define RAVI_TM_USERDATA (((ravi_type_map)1)<<RAVI_TI_USERDATA)
#define RAVI_TM_OTHER (((ravi_type_map)1)<<RAVI_TI_OTHER)
#define RAVI_TM_FALSISH (RAVI_TM_NIL | RAVI_TM_FALSE)
#define RAVI_TM_TRUISH (~RAVI_TM_FALSISH)
#define RAVI_TM_BOOLEAN (RAVI_TM_FALSE | RAVI_TM_TRUE)
#define RAVI_TM_NUMBER (RAVI_TM_INTEGER | RAVI_TM_FLOAT)
#define RAVI_TM_INDEXABLE (RAVI_TM_INTEGER_ARRAY | RAVI_TM_FLOAT_ARRAY | RAVI_TM_TABLE)
#define RAVI_TM_STRING_OR_NIL (RAVI_TM_STRING | RAVI_TM_NIL)
#define RAVI_TM_FUNCTION_OR_NIL (RAVI_TM_FUNCTION | RAVI_TM_NIL)
#define RAVI_TM_BOOLEAN_OR_NIL (RAVI_TM_BOOLEAN | RAVI_TM_NIL)
#define RAVI_TM_USERDATA_OR_NIL (RAVI_TM_USERDATA | RAVI_TM_NIL)
#define RAVI_TM_ANY (~0)
typedef enum {
RAVI_TNIL = RAVI_TM_NIL,           /* NIL */
RAVI_TNUMINT = RAVI_TM_INTEGER,    /* integer number */
RAVI_TNUMFLT = RAVI_TM_FLOAT,        /* floating point number */
RAVI_TNUMBER = RAVI_TM_NUMBER,
RAVI_TARRAYINT = RAVI_TM_INTEGER_ARRAY,      /* array of ints */
RAVI_TARRAYFLT = RAVI_TM_FLOAT_ARRAY,      /* array of doubles */
RAVI_TTABLE = RAVI_TM_TABLE,         /* Lua table */
RAVI_TSTRING = RAVI_TM_STRING_OR_NIL,        /* string */
RAVI_TFUNCTION = RAVI_TM_FUNCTION_OR_NIL,      /* Lua or C Function */
RAVI_TBOOLEAN = RAVI_TM_BOOLEAN_OR_NIL,       /* boolean */
RAVI_TTRUE = RAVI_TM_TRUE,
RAVI_TFALSE = RAVI_TM_FALSE,
RAVI_TUSERDATA = RAVI_TM_USERDATA_OR_NIL,      /* userdata or lightuserdata */
RAVI_TANY = RAVI_TM_ANY,      /* Lua dynamic type */
} ravitype_t;
typedef struct Upvaldesc {
	TString *name;
	TString *usertype;
	ravi_type_map ravi_type;
	lu_byte instack;
	lu_byte idx;
} Upvaldesc;
typedef struct LocVar {
	TString *varname;
	TString *usertype;
	int startpc;
	int endpc;
	ravi_type_map ravi_type;
} LocVar;
typedef enum {
	RAVI_JIT_NOT_COMPILED = 0,
	RAVI_JIT_CANT_COMPILE = 1,
	RAVI_JIT_COMPILED = 2
} ravi_jit_status_t;
typedef enum {
	RAVI_JIT_FLAG_NONE = 0,
	RAVI_JIT_FLAG_HASFORLOOP = 1
} ravi_jit_flag_t;
typedef struct RaviJITProto {
	lu_byte jit_status;
	lu_byte jit_flags;
	unsigned short execution_count;
	void *jit_data;
	lua_CFunction jit_function;
} RaviJITProto;
typedef struct Proto {
	CommonHeader;
	lu_byte numparams;
	lu_byte is_vararg;
	lu_byte maxstacksize;
	int sizeupvalues;
	int sizek;
	int sizecode;
	int sizelineinfo;
	int sizep;
	int sizelocvars;
	int linedefined;
	int lastlinedefined;
	TValue *k;
	Instruction *code;
	struct Proto **p;
	int *lineinfo;
	LocVar *locvars;
	Upvaldesc *upvalues;
	struct LClosure *cache;
	TString  *source;
	GCObject *gclist;
	RaviJITProto ravi_jit;
} Proto;
typedef struct UpVal UpVal;
#define ClosureHeader \
	CommonHeader; lu_byte nupvalues; GCObject *gclist
typedef struct CClosure {
	ClosureHeader;
	lua_CFunction f;
	TValue upvalue[1];
} CClosure;
typedef struct LClosure {
	ClosureHeader;
	struct Proto *p;
	UpVal *upvals[1];
} LClosure;
typedef union Closure {
	CClosure c;
	LClosure l;
} Closure;
#define isLfunction(o)	ttisLclosure(o)
#define getproto(o)	(clLvalue(o)->p)
typedef union TKey {
	struct {
		TValuefields;
		int next;
	} nk;
	TValue tvk;
} TKey;
#define setnodekey(L,key,obj) \
	{ TKey *k_=(key); const TValue *io_=(obj); \
	  k_->nk.value_ = io_->value_; k_->nk.tt_ = io_->tt_; \
	  (void)L; checkliveness(L,io_); }
typedef struct Node {
	TValue i_val;
	TKey i_key;
} Node;
typedef enum RaviArrayModifer {
 RAVI_ARRAY_SLICE = 1,
 RAVI_ARRAY_FIXEDSIZE = 2,
 RAVI_ARRAY_ALLOCATED = 4,
 RAVI_ARRAY_ISFLOAT = 8
} RaviArrayModifier;
enum {
 RAVI_ARRAY_MAX_INLINE = 3,
};
typedef struct RaviArray {
 CommonHeader;
 lu_byte flags;
 unsigned int len;
 unsigned int size;
 union {
  lua_Number numarray[RAVI_ARRAY_MAX_INLINE];
  lua_Integer intarray[RAVI_ARRAY_MAX_INLINE];
  struct RaviArray* parent;
 };
 char *data;
 struct Table *metatable;
} RaviArray;
typedef struct Table {
 CommonHeader;
 lu_byte flags;
 lu_byte lsizenode;
 unsigned int sizearray;
 TValue *array;
 Node *node;
 Node *lastfree;
 struct Table *metatable;
 GCObject *gclist;
 unsigned int hmask;
} Table;
typedef struct Mbuffer {
	char *buffer;
	size_t n;
	size_t buffsize;
} Mbuffer;
typedef struct stringtable {
	TString **hash;
	int nuse;
	int size;
} stringtable;
struct lua_Debug;
typedef intptr_t lua_KContext;
typedef int(*lua_KFunction)(struct lua_State *L, int status, lua_KContext ctx);
typedef void *(*lua_Alloc)(void *ud, void *ptr, size_t osize,
	size_t nsize);
typedef void(*lua_Hook)(struct lua_State *L, struct lua_Debug *ar);
typedef struct CallInfo {
	StkId func;
	StkId	top;
	struct CallInfo *previous, *next;
	union {
		struct {
			StkId base;
			const Instruction *savedpc;
		} l;
		struct {
			lua_KFunction k;
			ptrdiff_t old_errfunc;
			lua_KContext ctx;
		} c;
	} u;
	ptrdiff_t extra;
	short nresults;
	unsigned short callstatus;
	unsigned short stacklevel;
	lu_byte jitstatus;
   lu_byte magic;
} CallInfo;
#define CIST_OAH	(1<<0)
#define CIST_LUA	(1<<1)
#define CIST_HOOKED	(1<<2)
#define CIST_FRESH	(1<<3)
#define CIST_YPCALL	(1<<4)
#define CIST_TAIL	(1<<5)
#define CIST_HOOKYIELD	(1<<6)
#define CIST_LEQ	(1<<7)
#define CIST_FIN	(1<<8)
#define isLua(ci)	((ci)->callstatus & CIST_LUA)
#define isJITed(ci) ((ci)->jitstatus)
#define setoah(st,v)	((st) = ((st) & ~CIST_OAH) | (v))
#define getoah(st)	((st) & CIST_OAH)
typedef struct global_State global_State;
struct lua_State {
	CommonHeader;
	lu_byte status;
	StkId top;
	global_State *l_G;
	CallInfo *ci;
	const Instruction *oldpc;
	StkId stack_last;
	StkId stack;
	UpVal *openupval;
	GCObject *gclist;
	struct lua_State *twups;
	struct lua_longjmp *errorJmp;
	CallInfo base_ci;
	volatile lua_Hook hook;
	ptrdiff_t errfunc;
	int stacksize;
	int basehookcount;
	int hookcount;
	unsigned short nny;
	unsigned short nCcalls;
	lu_byte hookmask;
	lu_byte allowhook;
	unsigned short nci;
   lu_byte magic;
};
#define G(L)	(L->l_G)
union GCUnion {
	GCObject gc;
	struct TString ts;
	struct Udata u;
	union Closure cl;
	struct Table h;
   struct RaviArray arr;
	struct Proto p;
	struct lua_State th;
};
struct UpVal {
	TValue *v;
	lu_mem refcount;
	union {
		struct {
			UpVal *next;
			int touched;
		} open;
		TValue value;
	} u;
};
#define cast_u(o)	cast(union GCUnion *, (o))
#define gco2ts(o)  \
	check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))
#define gco2u(o)  check_exp((o)->tt == LUA_TUSERDATA, &((cast_u(o))->u))
#define gco2lcl(o)  check_exp((o)->tt == LUA_TLCL, &((cast_u(o))->cl.l))
#define gco2ccl(o)  check_exp((o)->tt == LUA_TCCL, &((cast_u(o))->cl.c))
#define gco2cl(o)  \
	check_exp(novariant((o)->tt) == LUA_TFUNCTION, &((cast_u(o))->cl))
#define gco2t(o)  check_exp((o)->tt == LUA_TTABLE, &((cast_u(o))->h))
#define gco2array(o)  check_exp(((o)->tt == RAVI_TIARRAY || (o)->tt == RAVI_TFARRAY), &((cast_u(o))->arr))
#define gco2p(o)  check_exp((o)->tt == LUA_TPROTO, &((cast_u(o))->p))
#define gco2th(o)  check_exp((o)->tt == LUA_TTHREAD, &((cast_u(o))->th))
#define obj2gco(v) \
	check_exp(novariant((v)->tt) < LUA_TDEADKEY, (&(cast_u(v)->gc)))
#define LUA_FLOORN2I		0
#define tonumber(o,n) \
  (ttisfloat(o) ? (*(n) = fltvalue(o), 1) : luaV_tonumber_(o,n))
#define tointeger(o,i) \
  (ttisinteger(o) ? (*(i) = ivalue(o), 1) : luaV_tointeger(o,i,LUA_FLOORN2I))
#define tointegerns(o, i) (ttisinteger(o) ? (*(i) = ivalue(o), 1) : luaV_tointegerns(o, i, LUA_FLOORN2I))
extern int luaV_tonumber_(const TValue *obj, lua_Number *n);
extern int luaV_tointeger(const TValue *obj, lua_Integer *p, int mode);
extern int luaV_tointegerns(const TValue *obj, lua_Integer *p, int mode);
extern void luaF_close (lua_State *L, StkId level);
extern int luaD_poscall (lua_State *L, CallInfo *ci, StkId firstResult, int nres);
extern void luaD_growstack (lua_State *L, int n);
extern int luaV_equalobj(lua_State *L, const TValue *t1, const TValue *t2);
extern int luaV_lessthan(lua_State *L, const TValue *l, const TValue *r);
extern int luaV_lessequal(lua_State *L, const TValue *l, const TValue *r);
extern void luaV_gettable (lua_State *L, const TValue *t, TValue *key, StkId val);
extern void luaV_settable (lua_State *L, const TValue *t, TValue *key, StkId val);
extern int luaV_execute(lua_State *L);
extern int luaD_precall (lua_State *L, StkId func, int nresults, int op_call);
extern void raviV_op_newtable(lua_State *L, CallInfo *ci, TValue *ra, int b, int c);
extern void raviV_op_newarrayint(lua_State *L, CallInfo *ci, TValue *ra);
extern void raviV_op_newarrayfloat(lua_State *L, CallInfo *ci, TValue *ra);
extern void luaO_arith (lua_State *L, int op, const TValue *p1, const TValue *p2, TValue *res);
extern void raviV_op_setlist(lua_State *L, CallInfo *ci, TValue *ra, int b, int c);
extern void raviV_op_concat(lua_State *L, CallInfo *ci, int a, int b, int c);
extern void raviV_op_closure(lua_State *L, CallInfo *ci, LClosure *cl, int a, int Bx);
extern void raviV_op_vararg(lua_State *L, CallInfo *ci, LClosure *cl, int a, int b);
extern void luaV_objlen (lua_State *L, StkId ra, const TValue *rb);
extern int luaV_forlimit(const TValue *obj, lua_Integer *p, lua_Integer step, int *stopnow);
extern void raviV_op_setupval(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_op_setupvali(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_op_setupvalf(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_op_setupvalai(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_op_setupvalaf(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_op_setupvalt(lua_State *L, LClosure *cl, TValue *ra, int b);
extern void raviV_raise_error(lua_State *L, int errorcode);
extern void raviV_raise_error_with_info(lua_State *L, int errorcode, const char *info);
extern void luaD_call (lua_State *L, StkId func, int nResults);
extern void raviH_set_int(lua_State *L, RaviArray *t, lua_Unsigned key, lua_Integer value);
extern void raviH_set_float(lua_State *L, RaviArray *t, lua_Unsigned key, lua_Number value);
extern int raviV_check_usertype(lua_State *L, TString *name, const TValue *o);
extern void luaT_trybinTM (lua_State *L, const TValue *p1, const TValue *p2, TValue *res, TMS event);
extern void raviV_gettable_sskey(lua_State *L, const TValue *t, TValue *key, TValue *val);
extern void raviV_settable_sskey(lua_State *L, const TValue *t, TValue *key, TValue *val);
extern void raviV_gettable_i(lua_State *L, const TValue *t, TValue *key, TValue *val);
extern void raviV_settable_i(lua_State *L, const TValue *t, TValue *key, TValue *val);
extern lua_Integer luaV_shiftl(lua_Integer x, lua_Integer y);
extern void ravi_dump_value(lua_State *L, const struct lua_TValue *v);
extern void raviV_op_bnot(lua_State *L, TValue *ra, TValue *rb);
extern void luaV_concat (lua_State *L, int total);
extern void *luaM_realloc_ (lua_State *L, void *block, size_t osize, size_t nsize);
extern LClosure *luaF_newLclosure (lua_State *L, int n);
extern TString *luaS_newlstr (lua_State *L, const char *str, size_t l);
extern Proto *luaF_newproto (lua_State *L);
extern void luaD_inctop (lua_State *L);
#define luaM_reallocv(L,b,on,n,e) luaM_realloc_(L, (b), (on)*(e), (n)*(e))
#define luaM_newvector(L,n,t) cast(t *, luaM_reallocv(L, NULL, 0, n, sizeof(t)))
#define R(i) (base + i)
#define K(i) (k + i)
#define S(i) (stackbase + i)
#define stackoverflow(L, n) (((int)(L->top - L->stack) + (n) + 5) >= L->stacksize)
#define savestack(L,p)		((char *)(p) - (char *)L->stack)
#define restorestack(L,n)	((TValue *)((char *)L->stack + (n)))
#define tonumberns(o,n) \
	(ttisfloat(o) ? ((n) = fltvalue(o), 1) : \
	(ttisinteger(o) ? ((n) = cast_num(ivalue(o)), 1) : 0))
#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))
#define nan (0./0.)
#define inf (1./0.)
#define luai_numunm(L,a)        (-(a))
static int __ravifunc_1(lua_State *L) {
int error_code = 0;
int result = 0;
CallInfo *ci = L->ci;
LClosure *cl = clLvalue(ci->func);
TValue *k = cl->p->k;
StkId base = ci->u.l.base;
lua_Integer i_0 = 0, i_1 = 0, i_2 = 0, i_3 = 0, i_4 = 0, i_5 = 0, i_6 = 0, i_7 = 0, i_8 = 0, i_9 = 0, i_10 = 0, i_11 = 0, i_12 = 0, i_13 = 0, i_14 = 0, i_15 = 0, i_16 = 0, i_17 = 0, i_18 = 0, i_19 = 0, i_20 = 0, i_21 = 0, i_22 = 0;
TValue ival0; settt_(&ival0, LUA_TNUMINT);
TValue fval0; settt_(&fval0, LUA_TNUMFLT);
TValue bval0; settt_(&bval0, LUA_TBOOLEAN);
TValue ival1; settt_(&ival1, LUA_TNUMINT);
TValue fval1; settt_(&fval1, LUA_TNUMFLT);
TValue bval1; settt_(&bval1, LUA_TBOOLEAN);
TValue nilval; setnilvalue(&nilval);
L0:
{
 TValue *tab = cl->upvals[0]->v;
 TValue *key = K(0);
 TValue *dst = R(1);
 raviV_gettable_sskey(L, tab, key, dst);
 base = ci->u.l.base;
}
{
 TValue *tab = R(1);
 TValue *key = K(1);
 TValue *dst = R(2);
 raviV_gettable_sskey(L, tab, key, dst);
 base = ci->u.l.base;
}
 if (stackoverflow(L,3)) { luaD_growstack(L, 3); base = ci->u.l.base; }
 L->top = R(2) + 2;
{
TValue *dst_reg = R(3);
setivalue(dst_reg, 8190);
}
{
 TValue *ra = R(2);
 int result = luaD_precall(L, ra, 1, 1);
 if (result) {
  if (result == 1 && 1 >= 0)
   L->top = ci->top;
 }
 else {  /* Lua function */
  result = luaV_execute(L);
  if (result) L->top = ci->top;
 }
 base = ci->u.l.base;
}
{
 TValue *ra = R(2);
 if (!ttisiarray(ra)) {
  error_code = 2;
  goto Lraise_error;
 }
}
{
const TValue *src_reg = R(2);
TValue *dst_reg = R(0);
dst_reg->tt_ = src_reg->tt_;
dst_reg->value_.n = src_reg->value_.n;
}
i_5 = 0;
i_6 = 100000;
i_7 = 1;
{ i_5 = i_5 - i_7; }
goto L2;
L1:
 return result;
Lraise_error:
 raviV_raise_error(L, error_code); /* does not return */
 return result;
L2:
{ i_5 = i_5 + i_7; }
goto L3;
L3:
{ i_8 = i_6 < i_5; }
{ if (i_8 != 0) goto L5; else goto L4; }
L4:
i_4 = i_5;
i_3 = 0;
i_10 = 0;
i_11 = 8190;
i_12 = 1;
{ i_10 = i_10 - i_12; }
goto L6;
L5:
{
 TValue *stackbase = ci->func;
 int wanted = ci->nresults;
 result = wanted == -1 ? 0 : 1;
 if (wanted == -1) wanted = 1;
 int j = 0;
 if (0 < wanted) {
{
TValue *dst_reg = S(0);
setivalue(dst_reg, i_3);
}
 }
 j++;
 while (j < wanted) {
  setnilvalue(S(j));
  j++;
 }
 L->top = S(0) + wanted;
 L->ci = ci->previous;
}
goto L1;
L6:
{ i_10 = i_10 + i_12; }
goto L7;
L7:
{ i_13 = i_11 < i_10; }
{ if (i_13 != 0) goto L9; else goto L8; }
L8:
i_9 = i_10;
{
 RaviArray *arr = arrvalue(R(0));
 lua_Unsigned ukey = (lua_Unsigned) i_9;
 lua_Integer *iptr = (lua_Integer *)arr->data;
 if (ukey < (lua_Unsigned)(arr->len)) {
 iptr[ukey] = 1;
} else {
 raviH_set_int(L, arr, ukey, 1);
}
}
goto L6;
L9:
i_11 = 0;
i_12 = 8190;
i_13 = 1;
{ i_11 = i_11 - i_13; }
goto L10;
L10:
{ i_11 = i_11 + i_13; }
goto L11;
L11:
{ i_9 = i_12 < i_11; }
{ if (i_9 != 0) goto L13; else goto L12; }
L12:
i_10 = i_11;
goto L14;
L13:
goto L2;
L14:
{
 RaviArray *arr = arrvalue(R(0));
 lua_Unsigned ukey = (lua_Unsigned) i_10;
 lua_Integer *iptr = (lua_Integer *)arr->data;
 i_14 = iptr[ukey];
}
{ i_15 = i_14 == 1; }
{ if (i_15 != 0) goto L15; else goto L16; }
L15:
{ i_14 = i_10 + i_10; }
{ i_16 = i_14 + 3; }
i_2 = i_16;
{ i_14 = i_10 + i_2; }
i_17 = i_14;
i_18 = 8190;
i_19 = i_2;
{ i_20 = 0 < i_19; }
{ i_17 = i_17 - i_19; }
goto L17;
L16:
goto L10;
L17:
{ i_17 = i_17 + i_19; }
{ if (i_20 != 0) goto L18; else goto L19; }
L18:
{ i_21 = i_18 < i_17; }
{ if (i_21 != 0) goto L21; else goto L20; }
L19:
{ i_21 = i_17 < i_18; }
{ if (i_21 != 0) goto L21; else goto L20; }
L20:
i_16 = i_17;
{
 RaviArray *arr = arrvalue(R(0));
 lua_Unsigned ukey = (lua_Unsigned) i_16;
 lua_Integer *iptr = (lua_Integer *)arr->data;
 if (ukey < (lua_Unsigned)(arr->len)) {
 iptr[ukey] = 0;
} else {
 raviH_set_int(L, arr, ukey, 0);
}
}
goto L17;
L21:
{ i_17 = i_3 + 1; }
i_3 = i_17;
goto L16;
}
EXPORT LClosure *mymain(lua_State *L) {
 LClosure *cl = luaF_newLclosure(L, 1);
 setclLvalue(L, L->top, cl);
 luaD_inctop(L);
 cl->p = luaF_newproto(L);
 Proto *f = cl->p;
 f->ravi_jit.jit_function = __ravifunc_1;
 f->ravi_jit.jit_status = RAVI_JIT_COMPILED;
 f->numparams = 0;
 f->is_vararg = 0;
 f->maxstacksize = 3;
 f->k = luaM_newvector(L, 2, TValue);
 f->sizek = 2;
 for (int i = 0; i < 2; i++)
  setnilvalue(&f->k[i]);
 {
  TValue *o = &f->k[0];
  setsvalue2n(L, o, luaS_newlstr(L, "table", 5));
 }
 {
  TValue *o = &f->k[1];
  setsvalue2n(L, o, luaS_newlstr(L, "intarray", 8));
 }
 f->upvalues = luaM_newvector(L, 1, Upvaldesc);
 f->sizeupvalues = 1;
 f->upvalues[0].instack = 0;
 f->upvalues[0].idx = 0;
 f->upvalues[0].name = NULL;
 f->upvalues[0].usertype = NULL;
 f->upvalues[0].ravi_type = 128;
 return cl;
}

