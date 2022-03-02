Uses CRT;
Const
     ProbePorts : Array [1..8] of Word =
     ($300,$310,$320,$330,$380,$390,$3A0,$3B0);
     FreqKHz : Array [0..1] of String = (' KHz', ' КГц');
     FreqHz  : Array [0..1] of String = (' Hz', ' Гц');
     WritePort : Array [0..1] of String = ('Write to port 0x','Записал в порт 0x');
     ByteStr   : Array [0..1] of String = (' byte 0x',' байт 0x');
     ToneHelp : Array [1..6,0..1] of String = (
     ('enable','пуск'),
     ('smooth atack','плавная атака'),
     ('low pass filter','ФНЧ'),
     ('low freq filter','вывод в НЧ канал'),
     ('med freq filter','вывод в СЧ канал'),
     ('high freq filter','вывод в ВЧ канал')
     );
     Ch7Help  : Array [1..5,0..1] of String = (
     ('enable','пуск'),
     ('duration','длительность'),
     ('amplitude/0/noise','амплитуда/0/шум'),
     ('amplitude/1','амплитуда/1'),
     ('amplitude/2','амплитуда/2')
     );
     Ch8Help   : Array [1..5,0..1] of String = (
     ('enable','пуск'),
     ('duration','длительность'),
     ('not used','не используется'),
     ('frequency/2','частота/2'),
     ('frequency/1','частота/1')
     );
Var
   Lang         : Byte;
   SndPort      : Word;
   t1,t2        : Byte;
   i, j         : Byte;
   OrigMode     : Integer;
   f            : longint;
   Key          : Char;
   ChDividers   : Array [1..6] of Word;
   ToneCh       : Array [1..8] of Byte;
   CurChan      : Byte;
   CurPos       : Byte;
   NeedUpdate   : Boolean;
   NeedChUpdate   : Boolean;
   CurPort      : Word;
   S            : String;

procedure ClearBit(var Value: Byte; Index: Byte);
begin
  Value:=  Value and not(1 shl Index);
end;

procedure SetBit(var Value: Byte; Index: Byte);
begin
  Value:=  Value or (1 shl Index);
end;

function GetBit(Value: Byte; Index: Byte): Boolean;
begin
  GetBit := ((Value shr Index) and 1) = 1;
end;

Function Dec2Hex(N: Word): String;
Const
     HD : Array [0..15] of Char = ('0','1','2','3','4','5','6','7',
                                   '8','9','A','B','C','D','E','F');
Begin
     Dec2Hex := (HD[Hi(N) shr 4]+HD[Hi(N) and $F])+
                (HD[Lo(N) shr 4]+HD[Lo(N) and $F]);
End;

Procedure DrawMenu;
Begin
     TextColor(Black);
     TextBackground(White);
     S := Dec2Hex(SndPort);
     Delete(S,1,1);
     If Lang = 0 then
         WriteLn(' Music cell control/test  | Port: 0x',S,' ')
     else
         WriteLn(' Тест музыкальной ячейки  | Порт: 0x',S,' ');
     TextBackground(Black);
     TextColor(White);
     If Lang = 0 then
         WriteLn('           PERIOD EN SA LP LF MF HF')
     else
         WriteLn('           ПЕРИОД EN ПА ФЛ НЧ СЧ ВЧ');
     For i := 1 to 6 do
         Begin
              TextColor(LightGray);
              If Lang = 0 then
                  WriteLn('   CHANNEL',i,' ')
              else
                  WriteLn('    КАНАЛ ',i,' ');
         End;
     WriteLn;
     WriteLn;
     TextColor(White);
     If Lang = 0 then
         WriteLn('                     EN DU F1 F2 F3')
     else
         WriteLn('                     ИМ ДЛ F1 F2 F3');
     For i := 1 to 2 do
     Begin
          TextColor(LightGray);
          If Lang = 0 then
             WriteLn('  CHANNEL ',i+6,'N')
          else
             WriteLn('    КАНАЛ ',i+6,'Ш');
     End;
End;

Begin
     Lang := 0; {english}
     SndPort := 0;
     For i := 1 to 8 do
         Begin
              Port[ProbePorts[i]+$0f] := $80; {disable GATE}
              Port[ProbePorts[i]+3] := $3E;
              Port[ProbePorts[i]+3] := $7E;
              Port[ProbePorts[i]+3] := $BE;
              Port[ProbePorts[i]] := $AA;
              Port[ProbePorts[i]] := $55;
              Port[ProbePorts[i]] := $AA;
              Port[ProbePorts[i]] := $55;
              Port[ProbePorts[i]] := $AA;
              Port[ProbePorts[i]] := $55;
              t1 := Port[ProbePorts[i]];
              t2 := Port[ProbePorts[i]];
              If (t1 = $AA) and (t2 = $55) then
                 Begin
                      SndPort := ProbePorts[i];
                      break;
                 End
              Else
                  Begin
                       S := Dec2Hex(ProbePorts[i]);
                       Delete(S,1,1);
                       WriteLn('Probe port 0x',S,' false [t1=',Dec2Hex(t1),'; t2=',Dec2Hex(t2),']');
                  End;
         End;
     If SndPort = 0 then
        Begin
             WriteLn('MUZ_CELL not found');
             Halt;
        End;
     Port[ProbePorts[i]+3] := $3E; {set up 1-st 8253}
     Port[ProbePorts[i]+3] := $7E;
     Port[ProbePorts[i]+3] := $BE;
     Port[ProbePorts[i]+7] := $3E; {set up 2-nd 8253}
     Port[ProbePorts[i]+7] := $7E;
     Port[ProbePorts[i]+7] := $BE;
     Port[ProbePorts[i]+$0f] := $0; {enable GATE}

     OrigMode := LastMode;
     TextMode(CO40);
     For i := 1 to 6 do
         Begin
              ChDividers[i] := $FFFE;
              ToneCh[i] := i;
         End;
     CurChan := 1;
     CurPos := 1;
     NeedUpdate := False;
     NeedChUpdate := False;
     DrawMenu;
     Repeat
           {Redraw tone channels}
           For i := 1 to 6 do
               Begin
                    GotoXY(13,i+3);
                    If i = CurChan then
                       TextColor(LightGray)
                    else
                        TextColor(DarkGray);
                    If (CurPos = 1) and (CurChan = i) then
                       Begin
                            TextBackground(White);
                            TextColor(DarkGray);
                       End;
                    Write(Dec2Hex(ChDividers[i]));
                    TextBackground(Black);
                    If i = CurChan then
                       TextColor(LightGray)
                    else
                        TextColor(DarkGray);
                    Write(' ');
                    For j := 5 downto 0 do
                        Begin
                             If i = CurChan then
                                TextColor(LightGray)
                             else
                                 TextColor(DarkGray);
                             Write('  ');
                             If (CurPos = 5-j+2) and (CurChan = i) then
                                Begin
                                     TextBackGround(White);
                                     TextColor(DarkGray);
                                End;
                             If GetBit(ToneCh[i],j) then Write('X') else Write('.');
                             TextBackGround(Black);
                             If i = CurChan then
                                TextColor(LightGray)
                             else
                                 TextColor(DarkGray);
                        End;
               End;
           {Redraw noice channels}
           For i := 7 to 8 do
               Begin
                    GotoXY(21,i+6);
                    For j := 4 downto 0 do
                        Begin
                             If i = CurChan then
                                TextColor(LightGray)
                             else
                                 TextColor(DarkGray);
                             Write('  ');
                             If (CurPos = 4-j+3) and (CurChan = i) then
                                Begin
                                     TextBackGround(White);
                                     TextColor(DarkGray);
                                End;
                             If GetBit(ToneCh[i],j) then Write('X') else Write('.');
                             TextBackGround(Black);
                             If i = CurChan then
                                TextColor(LightGray)
                             else
                                 TextColor(DarkGray);
                        End;
               End;

           {Print HELP screen}
           GotoXY(4,24);
           TextColor(DarkGray);
           If CurChan < 7 then
              Begin
                If Lang = 0 then
                   Write('Tone-channel ',CurChan,':                      ')
                else
                   Write('Тон-канал ',CurChan,':                         ');
              End
           else
              Begin
                If Lang = 0 then
                   Write('Noise-channel ',CurChan,':                     ')
                else
                   Write('Шум-канал ',CurChan,':                         ');
              End;

           If Lang = 0 then
              Begin
                   If CurChan > 6 then
                      GotoXY(21,24)
                   else
                      GotoXY(20,24);
              End
           else
               GotoXY(17,24);
           TextColor(LightGray);
           If CurPos = 1 then
              Begin
                   f := 1021429 div ChDividers[CurChan];
                   If f >= 1000 then
                      Write(f div 1000, FreqKhz[Lang])
                   else
                       Write(f,FreqHz[Lang]);
              End;
           If (CurChan < 7) and (CurPos <> 1) then
              Begin
                   Write(ToneHelp[CurPos-1][Lang]);
              End;
           If CurChan = 7 then Write(Ch7Help[CurPos-2][Lang]);
           If CurChan = 8 then Write(Ch8Help[CurPos-2][Lang]);

           If Not(NeedUpdate) and Not(NeedChUpdate) then
               Begin
                    GotoXY(6,18);
                    Write('                                  ');
                    GotoXY(6,19);
                    Write('                                  ');
               End;
           {Print out byte to channel control port, if needed}
           If NeedUpdate then
              Begin
                   j := 0;
                   For i := 6 downto 0 do
                       If GetBit(ToneCh[CurChan],i) then SetBit(j,i);
                   GotoXY(6,18);
                   CurPort := SndPort + CurChan +7;
                   S := Dec2Hex(CurPort);
                   Delete(S,1,1);
                   Write(WritePort[Lang],S);
                   S := Dec2Hex(j);
                   Delete(S,1,2);
                   Write(ByteStr[Lang],S);
                   NeedUpdate := False;
                   Port[CurPort] := j;
              End;

           {Print out byte to frequency port, if needed}
           If NeedChUpdate then
              Begin
                GotoXY(6,18);
                If CurChan < 4 then
                   CurPort := SndPort + (CurChan-1)
                Else
                   CurPort := SndPort + (CurChan);
                S := Dec2Hex(CurPort);
                Delete(S,1,1);
                Write(WritePort[Lang],S);
                S := Dec2Hex(Lo(ChDividers[CurChan]));
                Delete(S,1,2);
                Write(ByteStr[Lang],S);
                GotoXY(6,19);
                S := Dec2Hex(CurPort);
                Delete(S,1,1);
                Write(WritePort[Lang],S);
                S := Dec2Hex(Hi(ChDividers[CurChan]));
                Delete(S,1,2);
                Write(ByteStr[Lang],S);

                Port[CurPort] := Lo(ChDividers[CurChan]);
                Port[CurPort] := Hi(ChDividers[CurChan]);
                NeedChUpdate := False;
              End;

           If CurPos = 1 then
              GotoXY(13,CurChan+3)
           else
               If CurChan < 7 then
                  GotoXY((CurPos*3)+14,CurChan+3)
               else
                  GotoXY((CurPos*3)+14,CurChan+6);

           Key := ReadKey;
           If Key <> #0 then
              Begin
                   If Key = #32 then    {Space}
                      Begin
                           If (CurPos > 1) and not((CurChan =8) and (CurPos = 5)) then
                              Begin
                                   If GetBit(ToneCh[CurChan],7-CurPos) then
                                      ClearBit(ToneCh[CurChan],7-CurPos)
                                   else
                                       SetBit(ToneCh[CurChan],7-CurPos);
                                   NeedUpdate := True;
                              End;
                      End;
              End
           else
               Begin
                    Key:=ReadKey;
                    If Key = #80 then   {Down}
                       Begin
                            Inc(CurChan);
                            If (CurChan > 6) and (CurPos < 3) then CurPos := 3;
                            If CurChan > 8 then CurChan := 1;
                       End;
                    If Key = #72 then   {Up}
                       Begin
                            Dec(CurChan);
                            If CurChan < 1 then CurChan := 8;
                            If (CurChan > 6) and (CurPos < 3) then CurPos := 3;
                       End;
                    If Key = #77 then   {Right}
                       Begin
                            Inc(CurPos);
                            If CurPos > 7 then
                               If CurChan < 7 then
                                  CurPos := 1
                               else
                                  CurPos := 3;
                       End;
                    If Key = #75 then   {Left}
                       Begin
                            Dec(CurPos);
                            If CurChan < 7 then
                               If CurPos < 1 then CurPos := 7;
                            If CurChan > 6 then
                                If CurPos < 3 then CurPos := 7;
                       End;
                    If Key = #81 then   {PgDown}
                       Begin
                            If CurPos = 1 then
                               Begin
                                    Dec(ChDividers[CurChan],$100);
                                    NeedChUpdate := True;
                               End;
                       End;
                    If Key = #73 then   {PgUp}
                       Begin
                            If CurPos = 1 then
                               Begin
                                    Inc(ChDividers[CurChan],$100);
                                    NeedChUpdate := True;
                               End;
                       End;
               End;
     Until Key = #27;
     TextMode(OrigMode);
End.