program TLM_QNG_J;
{$mode objfpc}{$H+}
type
  U8=Byte; U32=LongWord; U64=QWord;
  TOp=(NOP,XOR64,AND64,SHL64,SHR64,ROT64,XTIME8X8,MIXCOL,FOLD64,RECURSE,EMIT);
  TInstr=packed record Op:TOp; D,A,B:U8; Imm:U32 end;
  TISA=array[0..31] of TInstr;
  TState=packed record Q0,Q1,Q2,Q3:U64; Depth:U32; Epoch:U64 end;
var ISA:TISA; S:TState;

function RotL64(X:U64; N:U8):U64;
begin if N=0 then Exit(X); RotL64:=(X shl N) or (X shr (64-N)) end;

function XTime(B:U8):U8;
var H:U8;
begin H:=B shr 7; B:=B shl 1; if H<>0 then B:=B xor $1B; XTime:=B end;

function MixColumn(A0,A1,A2,A3:U8):U32;
var T,X0,X1,X2,X3,Y0,Y1,Y2,Y3:U8;
begin
 T:=A0 xor A1 xor A2 xor A3;
 X0:=XTime(A0 xor A1); X1:=XTime(A1 xor A2);
 X2:=XTime(A2 xor A3); X3:=XTime(A3 xor A0);
 Y0:=A0 xor T xor X0; Y1:=A1 xor T xor X1;
 Y2:=A2 xor T xor X2; Y3:=A3 xor T xor X3;
 MixColumn:=(U32(Y0) shl 24) or (U32(Y1) shl 16) or
            (U32(Y2) shl 8) or U32(Y3)
end;

function Mix32(X:U32):U32;
begin Mix32:=MixColumn(U8(X shr 24),U8(X shr 16),U8(X shr 8),U8(X)) end;

function Mix64(X:U64):U64;
begin Mix64:=(U64(Mix32(U32(X shr 32))) shl 32) or U64(Mix32(U32(X))) end;

function Fold64(X:U64):U64;
begin X:=X xor(X shr 32); X:=X xor(X shr 16); X:=X xor(X shr 8);
 X:=X xor(X shr 4); Fold64:=X end;

function XTimeArray(X:U64):U64;
var I:Integer; R:U64;
begin R:=0; for I:=0 to 7 do R:=R or(U64(XTime(U8(X shr(I*8)))) shl(I*8));
 XTimeArray:=R end;

procedure LoadISA;
begin
 FillChar(ISA,SizeOf(ISA),0);
 ISA[0].Op:=XOR64; ISA[1].Op:=XTIME8X8; ISA[2].Op:=MIXCOL;
 ISA[3].Op:=ROT64; ISA[3].Imm:=7; ISA[4].Op:=XOR64; ISA[5].Op:=FOLD64;
 ISA[6].Op:=ROT64; ISA[6].Imm:=19; ISA[7].Op:=XOR64; ISA[8].Op:=MIXCOL;
 ISA[9].Op:=ROT64; ISA[9].Imm:=31; ISA[10].Op:=XOR64; ISA[11].Op:=FOLD64;
 ISA[12].Op:=ROT64; ISA[12].Imm:=43; ISA[13].Op:=XOR64; ISA[14].Op:=XTIME8X8;
 ISA[15].Op:=MIXCOL; ISA[16].Op:=ROT64; ISA[16].Imm:=13; ISA[17].Op:=XOR64;
 ISA[18].Op:=FOLD64; ISA[19].Op:=ROT64; ISA[19].Imm:=23; ISA[20].Op:=XOR64;
 ISA[21].Op:=MIXCOL; ISA[22].Op:=ROT64; ISA[22].Imm:=37; ISA[23].Op:=XOR64;
 ISA[24].Op:=XTIME8X8; ISA[25].Op:=FOLD64; ISA[26].Op:=ROT64; ISA[26].Imm:=47;
 ISA[27].Op:=XOR64; ISA[28].Op:=MIXCOL; ISA[29].Op:=ROT64; ISA[29].Imm:=53;
 ISA[30].Op:=XOR64; ISA[31].Op:=EMIT
end;

function Exec(var St:TState; I:TInstr):U64;
var A,B,R:U64;
begin
 A:=St.Q0; B:=St.Q1;
 case I.Op of
  NOP:R:=A; XOR64:R:=A xor B; AND64:R:=A and B;
  SHL64:R:=A shl(I.Imm and 63); SHR64:R:=A shr(I.Imm and 63);
  ROT64:R:=RotL64(A,I.Imm and 63); XTIME8X8:R:=XTimeArray(A);
  MIXCOL:R:=Mix64(A); FOLD64:R:=Fold64(A);
  RECURSE:R:=A xor RotL64(B,17); EMIT:R:=Fold64(A xor B)
 else R:=0 end;
 St.Q3:=St.Q2; St.Q2:=St.Q1; St.Q1:=St.Q0; St.Q0:=R; Inc(St.Epoch);
 Exec:=R
end;

procedure InitState(var St:TState; Seed:U64);
begin
 St.Q0:=Seed; St.Q1:=RotL64(Seed,11); St.Q2:=RotL64(Seed,23);
 St.Q3:=RotL64(Seed,37); St.Depth:=0; St.Epoch:=0
end;

function ISAFrame(var St:TState; Source:U64):U64;
var I:Integer; R:U64;
begin
 St.Q0:=St.Q0 xor Source; R:=Source;
 for I:=0 to 31 do begin
  R:=Exec(St,ISA[I]);
  if(I and 1)=0 then St.Q0:=St.Q0 xor RotL64(R,U8(I+1))
  else St.Q1:=St.Q1 xor RotL64(R,U8((I+3) and 63))
 end;
 ISAFrame:=Fold64(St.Q0 xor St.Q1 xor St.Q2 xor St.Q3)
end;

function QuantumNumber(var St:TState; Source:U64; Depth:U32):U64;
var R:U64;
begin
 if Depth=0 then begin QuantumNumber:=Fold64(St.Q0 xor St.Q1 xor Source); Exit end;
 R:=ISAFrame(St,Source xor U64(Depth));
 St.Q1:=St.Q1 xor RotL64(R,7); St.Q2:=St.Q2 xor RotL64(R,19);
 St.Q3:=St.Q3 xor RotL64(R,31);
 QuantumNumber:=QuantumNumber(St,R xor Source,Depth-1)
end;

function Generate(Source:U64; Depth:U32):U64;
begin InitState(S,Source); S.Depth:=Depth; Generate:=QuantumNumber(S,Source,Depth) end;

procedure Assert32(Name:String; Got,Expected:U32);
begin
 if Got<>Expected then begin WriteLn('FAIL ',Name,' got=',HexStr(Got,8),
 ' expected=',HexStr(Expected,8)); Halt(1) end
end;

procedure Verify;
begin
 if XTime($00)<>$00 then Halt(10); if XTime($01)<>$02 then Halt(11);
 if XTime($7F)<>$FE then Halt(12); if XTime($80)<>$1B then Halt(13);
 if XTime($FF)<>$E5 then Halt(14); if XTime($57)<>$AE then Halt(15);
 Assert32('D4 BF 5D 30',MixColumn($D4,$BF,$5D,$30),$046681E5);
 Assert32('ZERO',MixColumn($00,$00,$00,$00),$00000000);
 Assert32('REPEATED',MixColumn($50,$50,$50,$50),$50505050)
end;

procedure Dump(Name:String; X:U64);
begin WriteLn(Name,'=',HexStr(X,16)) end;

var Q:U64;
begin
 LoadISA; Verify; WriteLn('P3/P2 CHECK: PASS');
 Q:=Generate($D4BF5D30E5A1C379,16); Dump('Q0',Q);
 Q:=Generate($0000000000000000,16); Dump('QZERO',Q);
 Q:=Generate($FFFFFFFFFFFFFFFF,16); Dump('QONES',Q);
 Q:=Generate($8000000000000001,8); Dump('QBOUNDARY',Q);
 WriteLn('TLM QUANTUM NUMBER ISA/PASCAL COMPLETE')
end.
