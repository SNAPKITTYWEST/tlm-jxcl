program TLM_Quantum_Number_Generator;

{$mode objfpc}
{$H+}

{======================================================================
  TLM QUANTUM NUMBER GENERATOR
  Recursive vacuum / black-hole gravity input layer
======================================================================}

const

  WORD_BITS = 32;
  BYTE_BITS = 8;
  WORD_MASK = $FFFFFFFF;
  BYTE_MASK = $FF;

  MAX_RECURSION = 64;
  STATE_WORDS = 16;
  OUTPUT_WORDS = 256;

  PRIME_1 = $9E3779B9;
  PRIME_2 = $7F4A7C15;
  PRIME_3 = $F39CC060;
  PRIME_4 = $106AA070;

  GOLDEN_RATIO = $9E3779B9;

type

  TWord = Cardinal;

  TByte = Byte;

  TByteVector = array[0..3] of TByte;

  TWordState = array[0..STATE_WORDS - 1] of TWord;

  TOutputBuffer = array[0..OUTPUT_WORDS - 1] of TWord;

  TVacuumSample = record
    SourceWord : TWord;
    GravityWord : TWord;
    EventWord : TWord;
    ModeWord : TWord;
  end;

  TQuantumState = record
    State : TWordState;
    Depth : Cardinal;
    Index : Cardinal;
    LastWord : TWord;
    Accumulator : TWord;
  end;

{======================================================================
  BASIC BOOLEAN PRIMITIVES
======================================================================}

function XOR32(A, B: TWord): TWord;
begin
  XOR32 := A xor B;
end;

function AND32(A, B: TWord): TWord;
begin
  AND32 := A and B;
end;

function OR32(A, B: TWord): TWord;
begin
  OR32 := A or B;
end;

function NOT32(A: TWord): TWord;
begin
  NOT32 := not A;
end;

function XOR8(A, B: TByte): TByte;
begin
  XOR8 := A xor B;
end;

function AND8(A, B: TByte): TByte;
begin
  AND8 := A and B;
end;

{======================================================================
  ROTATION PRIMITIVES
======================================================================}

function ROTL32(A: TWord; N: Cardinal): TWord;
begin
  N := N mod 32;

  if N = 0 then
    ROTL32 := A
  else
    ROTL32 := (A shl N) or (A shr (32 - N));
end;

function ROTR32(A: TWord; N: Cardinal): TWord;
begin
  N := N mod 32;

  if N = 0 then
    ROTR32 := A
  else
    ROTR32 := (A shr N) or (A shl (32 - N));
end;

{======================================================================
  MODULAR INTEGER PRIMITIVES
======================================================================}

function Add32(A, B: TWord): TWord;
begin
  Add32 := (A + B) and WORD_MASK;
end;

function Sub32(A, B: TWord): TWord;
begin
  Sub32 := (A - B) and WORD_MASK;
end;

function Mul32(A, B: TWord): TWord;
begin
  Mul32 := (A * B) and WORD_MASK;
end;

{======================================================================
  BYTE EXTRACTION
======================================================================}

function Byte0(A: TWord): TByte;
begin
  Byte0 := TByte(A and $FF);
end;

function Byte1(A: TWord): TByte;
begin
  Byte1 := TByte((A shr 8) and $FF);
end;

function Byte2(A: TWord): TByte;
begin
  Byte2 := TByte((A shr 16) and $FF);
end;

function Byte3(A: TWord): TByte;
begin
  Byte3 := TByte((A shr 24) and $FF);
end;

function PackBytes(A, B, C, D: TByte): TWord;
begin
  PackBytes :=
    TWord(A) or
    (TWord(B) shl 8) or
    (TWord(C) shl 16) or
    (TWord(D) shl 24);
end;

{======================================================================
  GF(2^8) XTIME
======================================================================}

function XTime(B: TByte): TByte;
var
  S : TByte;
begin
  S := TByte((B shl 1) and BYTE_MASK);

  if (B and $80) <> 0 then
    S := S xor $1B;

  XTime := S;
end;

{======================================================================
  STRUCTURAL BYTE MIX
======================================================================}

function MixColumn(A0, A1, A2, A3: TByte): TByteVector;
var
  T : TByte;
  X0 : TByte;
  X1 : TByte;
  X2 : TByte;
  X3 : TByte;
  R : TByteVector;
begin

  T := A0 xor A1 xor A2 xor A3;

  X0 := XTime(A0 xor A1);
  X1 := XTime(A1 xor A2);
  X2 := XTime(A2 xor A3);
  X3 := XTime(A3 xor A0);

  R[0] := A0 xor T xor X0;
  R[1] := A1 xor T xor X1;
  R[2] := A2 xor T xor X2;
  R[3] := A3 xor T xor X3;

  MixColumn := R;
end;

{======================================================================
  32-BIT MIX COLUMN
======================================================================}

function MixWord(A: TWord): TWord;
var
  R : TByteVector;
begin

  R := MixColumn(
    Byte0(A),
    Byte1(A),
    Byte2(A),
    Byte3(A)
  );

  MixWord := PackBytes(
    R[0],
    R[1],
    R[2],
    R[3]
  );
end;

{======================================================================
  GRAVITY FIELD MIX
======================================================================}

function GravityMix(A, G: TWord): TWord;
var
  X : TWord;
begin

  X := XOR32(A, G);
  X := ROTL32(X, 7);

  X := Add32(
    X,
    Mul32(G, PRIME_1)
  );

  X := XOR32(
    X,
    ROTR32(G, 11)
  );

  GravityMix := X;
end;

{======================================================================
  VACUUM FIELD MIX
======================================================================}

function VacuumMix(A, V: TWord): TWord;
var
  X : TWord;
begin

  X := XOR32(A, V);

  X := ROTR32(X, 3);

  X := XOR32(
    X,
    ROTL32(V, 13)
  );

  X := Add32(
    X,
    PRIME_2
  );

  VacuumMix := X;
end;

{======================================================================
  MODE MIX
======================================================================}

function ModeMix(A, M: TWord): TWord;
var
  X : TWord;
begin

  X := XOR32(A, M);

  X := ROTL32(X, 17);

  X := Add32(
    X,
    PRIME_3
  );

  X := XOR32(
    X,
    ROTR32(M, 9)
  );

  ModeMix := X;
end;

{======================================================================
  EVENT MIX
======================================================================}

function EventMix(A, E: TWord): TWord;
var
  X : TWord;
begin

  X := Add32(A, E);

  X := ROTR32(X, 5);

  X := XOR32(
    X,
    ROTL32(E, 19)
  );

  X := Add32(
    X,
    PRIME_4
  );

  EventMix := X;
end;

{======================================================================
  SOURCE FUSION
======================================================================}

function FuseVacuumSample(
  const S : TVacuumSample
): TWord;
var
  X : TWord;
begin

  X := XOR32(
    S.SourceWord,
    S.GravityWord
  );

  X := XOR32(
    X,
    S.EventWord
  );

  X := XOR32(
    X,
    S.ModeWord
  );

  X := GravityMix(
    X,
    S.GravityWord
  );

  X := VacuumMix(
    X,
    S.SourceWord
  );

  X := EventMix(
    X,
    S.EventWord
  );

  X := ModeMix(
    X,
    S.ModeWord
  );

  FuseVacuumSample := X;
end;

{======================================================================
  QUANTUM NUMBER NORMALIZATION
======================================================================}

function NormalizeQuantumNumber(A: TWord): TWord;
var
  X : TWord;
begin

  X := A;

  X := XOR32(
    X,
    ROTR32(X, 16)
  );

  X := XOR32(
    X,
    ROTL32(X, 7)
  );

  X := Add32(
    X,
    GOLDEN_RATIO
  );

  NormalizeQuantumNumber := X;
end;

{======================================================================
  RECURSIVE SEED TRANSFORMATION
======================================================================}

function RecursiveSeed(
  Seed : TWord;
  Depth : Cardinal
): TWord;
var
  X : TWord;
begin

  if Depth = 0 then
  begin
    RecursiveSeed := Seed;
    Exit;
  end;

  X := ROTL32(
    Seed,
    (Depth mod 31) + 1
  );

  X := XOR32(
    X,
    PRIME_1 xor Depth
  );

  X := Add32(
    X,
    Mul32(
      Depth + 1,
      PRIME_2
    )
  );

  X := MixWord(X);

  X := XOR32(
    X,
    RecursiveSeed(
      X,
      Depth - 1
    )
  );

  RecursiveSeed := NormalizeQuantumNumber(X);
end;

{======================================================================
  RECURSIVE VACUUM STEP
======================================================================}

function RecursiveVacuumStep(
  Current : TWord;
  Sample : TVacuumSample;
  Depth : Cardinal
): TWord;
var
  X : TWord;
  R : TWord;
begin

  X := FuseVacuumSample(Sample);

  X := XOR32(
    X,
    Current
  );

  X := GravityMix(
    X,
    Sample.GravityWord
  );

  X := VacuumMix(
    X,
    Sample.SourceWord
  );

  X := EventMix(
    X,
    Sample.EventWord
  );

  X := ModeMix(
    X,
    Sample.ModeWord
  );

  X := MixWord(X);

  X := XOR32(
    X,
    RecursiveSeed(
      X xor Current,
      Depth mod MAX_RECURSION
    )
  );

  R := NormalizeQuantumNumber(X);

  RecursiveVacuumStep := R;
end;

{======================================================================
  QUANTUM STATE INITIALIZATION
======================================================================}

procedure InitializeQuantumState(
  var Q : TQuantumState;
  Seed : TWord
);
var
  I : Integer;
begin

  Q.Depth := 0;
  Q.Index := 0;
  Q.LastWord := Seed;
  Q.Accumulator := Seed;

  for I := 0 to STATE_WORDS - 1 do
  begin
    Q.State[I] :=
      RecursiveSeed(
        Seed xor
        TWord(I * PRIME_1),
        (I mod 16) + 1
      );
  end;

end;

{======================================================================
  STATE ROTATION
======================================================================}

procedure RotateQuantumState(
  var Q : TQuantumState
);
var
  I : Integer;
  T : TWord;
begin

  T := Q.State[STATE_WORDS - 1];

  for I := STATE_WORDS - 1 downto 1 do
    Q.State[I] := Q.State[I - 1];

  Q.State[0] := T;

end;

{======================================================================
  STATE DIFFUSION
======================================================================}

procedure DiffuseQuantumState(
  var Q : TQuantumState
);
var
  I : Integer;
  A : TWord;
  B : TWord;
begin

  for I := 0 to STATE_WORDS - 1 do
  begin

    A := Q.State[I];

    B := Q.State[
      (I + 1) mod STATE_WORDS
    ];

    A := XOR32(
      A,
      ROTL32(B, (I + 3) mod 32)
    );

    A := Add32(
      A,
      PRIME_1 xor TWord(I)
    );

    A := MixWord(A);

    Q.State[I] := A;

  end;

end;

{======================================================================
  STATE ACCUMULATOR
======================================================================}

procedure Accumulate(
  var Q : TQuantumState;
  V : TWord
);
begin

  Q.Accumulator :=
    XOR32(
      Q.Accumulator,
      V
    );

  Q.Accumulator :=
    ROTL32(
      Q.Accumulator,
      9
    );

  Q.Accumulator :=
    Add32(
      Q.Accumulator,
      PRIME_3
    );

end;

{======================================================================
  SAMPLE INJECTION
======================================================================}

procedure InjectSample(
  var Q : TQuantumState;
  S : TVacuumSample
);
var
  I : Integer;
  X : TWord;
begin

  X := FuseVacuumSample(S);

  X := XOR32(
    X,
    Q.Accumulator
  );

  for I := 0 to STATE_WORDS - 1 do
  begin

    Q.State[I] :=
      XOR32(
        Q.State[I],
        ROTL32(
          X,
          (I * 3 + Q.Depth) mod 32
        )
      );

  end;

  Accumulate(Q, X);

end;

{======================================================================
  RECURSIVE QUANTUM STEP
======================================================================}

function QuantumStep(
  var Q : TQuantumState;
  S : TVacuumSample
): TWord;
var
  X : TWord;
  I : Integer;
begin

  InjectSample(Q, S);

  RotateQuantumState(Q);

  DiffuseQuantumState(Q);

  X := Q.State[0];

  for I := 1 to STATE_WORDS - 1 do
  begin

    X := XOR32(
      X,
      ROTL32(
        Q.State[I],
        (I + Q.Depth) mod 32
      )
    );

  end;

  X := RecursiveVacuumStep(
    X,
    S,
    (Q.Depth mod MAX_RECURSION) + 1
  );

  Q.State[0] :=
    XOR32(
      Q.State[0],
      X
    );

  Q.LastWord := NormalizeQuantumNumber(X);

  Q.Depth := Q.Depth + 1;
  Q.Index := Q.Index + 1;

  Accumulate(
    Q,
    Q.LastWord
  );

  QuantumStep := Q.LastWord;

end;

{======================================================================
  QUANTUM NUMBER EXTRACTION
======================================================================}

function ExtractQuantumNumber(
  var Q : TQuantumState
): TWord;
var
  X : TWord;
begin

  X := Q.LastWord;

  X := XOR32(
    X,
    Q.Accumulator
  );

  X := XOR32(
    X,
    Q.State[
      Q.Index mod STATE_WORDS
    ]
  );

  X := MixWord(X);

  X := NormalizeQuantumNumber(X);

  ExtractQuantumNumber := X;

end;

{======================================================================
  QUANTUM MODE NUMBER
======================================================================}

function QuantumMode(
  QN : TWord
): TWord;
begin

  QuantumMode :=
    QN and $0000FFFF;

end;

{======================================================================
  QUANTUM OCCUPATION NUMBER
======================================================================}

function QuantumOccupation(
  QN : TWord
): TWord;
begin

  QuantumOccupation :=
    (QN shr 16) and $0000FFFF;

end;

{======================================================================
  QUANTUM PARITY
======================================================================}

function QuantumParity(
  QN : TWord
): TByte;
var
  X : TWord;
begin

  X := QN;

  X := X xor (X shr 16);
  X := X xor (X shr 8);
  X := X xor (X shr 4);
  X := X xor (X shr 2);
  X := X xor (X shr 1);

  QuantumParity :=
    TByte(X and 1);

end;

{======================================================================
  SOURCE CONSTRUCTION
======================================================================}

function MakeVacuumSample(
  SourceWord : TWord;
  GravityWord : TWord;
  EventWord : TWord;
  ModeWord : TWord
): TVacuumSample;
var
  S : TVacuumSample;
begin

  S.SourceWord := SourceWord;
  S.GravityWord := GravityWord;
  S.EventWord := EventWord;
  S.ModeWord := ModeWord;

  MakeVacuumSample := S;

end;

{======================================================================
  RECURSIVE GENERATOR
======================================================================}

function GenerateQuantumNumber(
  var Q : TQuantumState;
  S : TVacuumSample
): TWord;
var
  X : TWord;
begin

  X := QuantumStep(
    Q,
    S
  );

  X := ExtractQuantumNumber(Q);

  GenerateQuantumNumber := X;

end;

{======================================================================
  SEQUENCE GENERATOR
======================================================================}

procedure GenerateSequence(
  var Q : TQuantumState;
  const Samples : array of TVacuumSample;
  var Outputs : TOutputBuffer;
  Count : Cardinal
);
var
  I : Cardinal;
begin

  if Count > OUTPUT_WORDS then
    Count := OUTPUT_WORDS;

  for I := 0 to Count - 1 do
  begin

    Outputs[I] :=
      GenerateQuantumNumber(
        Q,
        Samples[I]
      );

  end;

end;

{======================================================================
  HEX OUTPUT
======================================================================}

procedure PrintHex32(
  A : TWord
);
begin

  Write(
    IntToHex(A, 8)
  );

end;

{======================================================================
  SAMPLE DISPLAY
======================================================================}

procedure DisplayQuantumNumber(
  N : TWord
);
begin

  WriteLn(
    'QUANTUM_NUMBER = ',
    IntToHex(N, 8)
  );

  WriteLn(
    'MODE = ',
    IntToHex(QuantumMode(N), 4)
  );

  WriteLn(
    'OCCUPATION = ',
    IntToHex(QuantumOccupation(N), 4)
  );

  WriteLn(
    'PARITY = ',
    QuantumParity(N)
  );

end;

{======================================================================
  DETERMINISTIC SOURCE VECTOR
======================================================================}

function DeterministicSource(
  Index : Cardinal
): TVacuumSample;
var
  S : TVacuumSample;
  X : TWord;
begin

  X :=
    Add32(
      PRIME_1,
      TWord(Index)
    );

  X :=
    RecursiveSeed(
      X,
      (Index mod 8) + 1
    );

  S.SourceWord :=
    X;

  S.GravityWord :=
    GravityMix(
      X,
      PRIME_2 xor Index
    );

  S.EventWord :=
    EventMix(
      X,
      PRIME_3 xor Index
    );

  S.ModeWord :=
    ModeMix(
      X,
      PRIME_4 xor Index
    );

  DeterministicSource := S;

end;

{======================================================================
  P3 COLUMN PREPARATION
======================================================================}

procedure QuantumWordToColumn(
  QN : TWord;
  var A0 : TByte;
  var A1 : TByte;
  var A2 : TByte;
  var A3 : TByte
);
begin

  A0 := Byte0(QN);
  A1 := Byte1(QN);
  A2 := Byte2(QN);
  A3 := Byte3(QN);

end;

{======================================================================
  P3 COLUMN ROUND
======================================================================}

function QuantumP3Round(
  QN : TWord
): TWord;
var
  A0 : TByte;
  A1 : TByte;
  A2 : TByte;
  A3 : TByte;
  R : TByteVector;
begin

  QuantumWordToColumn(
    QN,
    A0,
    A1,
    A2,
    A3
  );

  R :=
    MixColumn(
      A0,
      A1,
      A2,
      A3
    );

  QuantumP3Round :=
    PackBytes(
      R[0],
      R[1],
      R[2],
      R[3]
    );

end;

{======================================================================
  RECURSIVE P3 FEEDBACK
======================================================================}

function RecursiveP3(
  Value : TWord;
  Depth : Cardinal
): TWord;
var
  X : TWord;
begin

  if Depth = 0 then
  begin
    RecursiveP3 := Value;
    Exit;
  end;

  X :=
    QuantumP3Round(Value);

  X :=
    XOR32(
      X,
      Value
    );

  X :=
    ROTL32(
      X,
      (Depth mod 31) + 1
    );

  RecursiveP3 :=
    RecursiveP3(
      X,
      Depth - 1
    );

end;

{======================================================================
  FINAL QUANTUM NUMBER
======================================================================}

function FinalQuantumNumber(
  Raw : TWord
): TWord;
var
  X : TWord;
begin

  X :=
    RecursiveP3(
      Raw,
      4
    );

  X :=
    NormalizeQuantumNumber(
      X
    );

  FinalQuantumNumber := X;

end;

{======================================================================
  FULL GENERATOR PIPELINE
======================================================================}

function FullQuantumGeneration(
  var Q : TQuantumState;
  S : TVacuumSample
): TWord;
var
  Raw : TWord;
  Final : TWord;
begin

  Raw :=
    GenerateQuantumNumber(
      Q,
      S
    );

  Final :=
    FinalQuantumNumber(
      Raw
    );

  FullQuantumGeneration :=
    Final;

end;

{======================================================================
  VERIFICATION: XTIME
======================================================================}

procedure TestXTime;
begin

  if XTime($00) <> $00 then
    Halt(1);

  if XTime($01) <> $02 then
    Halt(2);

  if XTime($40) <> $80 then
    Halt(3);

  if XTime($80) <> $1B then
    Halt(4);

  if XTime($FF) <> $E5 then
    Halt(5);

end;

{======================================================================
  VERIFICATION: MIX COLUMN
======================================================================}

procedure TestMixColumn;
var
  R : TByteVector;
begin

  R :=
    MixColumn(
      $D4,
      $BF,
      $5D,
      $30
    );

  if R[0] <> $04 then
    Halt(10);

  if R[1] <> $66 then
    Halt(11);

  if R[2] <> $81 then
    Halt(12);

  if R[3] <> $E5 then
    Halt(13);

end;

{======================================================================
  VERIFICATION: ZERO COLUMN
======================================================================}

procedure TestZeroColumn;
var
  R : TByteVector;
begin

  R :=
    MixColumn(
      $00,
      $00,
      $00,
      $00
    );

  if R[0] <> $00 then
    Halt(20);

  if R[1] <> $00 then
    Halt(21);

  if R[2] <> $00 then
    Halt(22);

  if R[3] <> $00 then
    Halt(23);

end;

{======================================================================
  VERIFICATION: REPEATED COLUMN
======================================================================}

procedure TestRepeatedColumn;
var
  R : TByteVector;
begin

  R :=
    MixColumn(
      $50,
      $50,
      $50,
      $50
    );

  if R[0] <> $50 then
    Halt(30);

  if R[1] <> $50 then
    Halt(31);

  if R[2] <> $50 then
    Halt(32);

  if R[3] <> $50 then
    Halt(33);

end;

{======================================================================
  STATE VERIFICATION
======================================================================}

procedure TestStateInitialization;
var
  Q1 : TQuantumState;
  Q2 : TQuantumState;
  I : Integer;
begin

  InitializeQuantumState(
    Q1,
    $12345678
  );

  InitializeQuantumState(
    Q2,
    $12345678
  );

  for I := 0 to STATE_WORDS - 1 do
  begin

    if Q1.State[I] <> Q2.State[I] then
      Halt(40 + I);

  end;

end;

{======================================================================
  RECURSION VERIFICATION
======================================================================}

procedure TestRecursion;
var
  A : TWord;
  B : TWord;
begin

  A :=
    RecursiveSeed(
      $12345678,
      8
    );

  B :=
    RecursiveSeed(
      $12345678,
      8
    );

  if A <> B then
    Halt(60);

end;

{======================================================================
  SAMPLE VERIFICATION
======================================================================}

procedure TestSampleFusion;
var
  S : TVacuumSample;
  A : TWord;
  B : TWord;
begin

  S :=
    MakeVacuumSample(
      $11223344,
      $55667788,
      $99AABBCC,
      $DDEEFF00
    );

  A :=
    FuseVacuumSample(S);

  B :=
    FuseVacuumSample(S);

  if A <> B then
    Halt(70);

end;

{======================================================================
  COMPLETE VERIFICATION
======================================================================}

procedure RunVerification;
begin

  WriteLn(
    'TLM QUANTUM NUMBER GENERATOR'
  );

  WriteLn(
    'BEGIN VERIFICATION'
  );

  TestXTime;

  WriteLn(
    'XTIME TEST PASSED'
  );

  TestMixColumn;

  WriteLn(
    'MIXCOLUMN D4 BF 5D 30 TEST PASSED'
  );

  TestZeroColumn;

  WriteLn(
    'ZERO COLUMN TEST PASSED'
  );

  TestRepeatedColumn;

  WriteLn(
    'REPEATED COLUMN TEST PASSED'
  );

  TestStateInitialization;

  WriteLn(
    'STATE INITIALIZATION TEST PASSED'
  );

  TestRecursion;

  WriteLn(
    'RECURSION TEST PASSED'
  );

  TestSampleFusion;

  WriteLn(
    'SOURCE FUSION TEST PASSED'
  );

  WriteLn(
    'ALL VERIFICATION TESTS PASSED'
  );

end;

{======================================================================
  MAIN
======================================================================}

var

  Q : TQuantumState;

  S : TVacuumSample;

  I : Integer;

  N : TWord;

begin

  RunVerification;

  WriteLn;

  InitializeQuantumState(
    Q,
    $A5A5A5A5
  );

  WriteLn(
    'RECURSIVE QUANTUM GENERATION'
  );

  WriteLn(
    '-----------------------------'
  );

  for I := 0 to 15 do
  begin

    S :=
      DeterministicSource(
        I
      );

    N :=
      FullQuantumGeneration(
        Q,
        S
      );

    WriteLn(
      'EVENT ',
      I:3,
      ' SOURCE=',
      IntToHex(S.SourceWord, 8),
      ' GRAVITY=',
      IntToHex(S.GravityWord, 8),
      ' QUANTUM=',
      IntToHex(N, 8)
    );

  end;

  WriteLn;

  S :=
    MakeVacuumSample(
      $D4BF5D30,
      $00000000,
      $00000000,
      $00000000
    );

  N :=
    FullQuantumGeneration(
      Q,
      S
    );

  WriteLn(
    'FINAL QUANTUM NUMBER'
  );

  DisplayQuantumNumber(N);

  WriteLn;

  WriteLn(
    'P3 PROJECTION'
  );

  WriteLn(
    'INPUT = ',
    IntToHex(N, 8)
  );

  WriteLn(
    'OUTPUT = ',
    IntToHex(
      QuantumP3Round(N),
      8
    )
  );

  WriteLn;

  WriteLn(
    'GENERATOR COMPLETE'
  );

end.
