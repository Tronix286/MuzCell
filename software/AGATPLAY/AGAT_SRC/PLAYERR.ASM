		.errfile
		.objfile	"playerr.o"
		.cpu		"6502"

ZEROPAGE_START	.equ	$00
SKIP_TO_EVENT	.equ	$03

; Рабочие переменные процедуры расстройки
; ?? почему-то старший байт первый
DETUNE_HI	.equ	$C5
DETUNE_LO	.equ	$C6
TEMP_VAL	.equ	$CB

; Указатель на таблицу высот нот текущего мелодического канала
PITCH_TBL_PTR	.equ	$CD

; Номер текущего трека, помноженный на два
CUR_TRACK_X2	.equ	$CF

; Действие, которое обработчик прерывания должен выполнить над каналом
; 0 - ничего не делать,
; 1 - задать параметры атаки,
; -1 - задать параметры затухания
; После выполнения сбрасывается в 0
CHN1_ACTION	.equ	$D8
CHN2_ACTION	.equ	$D9
CHN3_ACTION	.equ	$DA
CHN4_ACTION	.equ	$DB
CHN5_ACTION	.equ	$DC
DRUM1_ACTION	.equ	$DD
DRUM2_ACTION	.equ	$DE

; Номер текущего трека
CURRENT_TRACK	.equ	$DF

; 7 двухбайтных указателей на текущие позиции нотного текста треков
TRACK1_PTR	.equ	$E0

; Указатель на таблицу длительностей текущего канала
DURAT_TBL_PTR	.equ	$EE

; 7 однобайтовых флагов воспроизведения треков
; 1 - трек остановлен
TRACK1_STOPPED	.equ	$F0

; Длительность интервала до следующего прерывания
NEXT_IRQ_TIMELO	.equ	$F7
NEXT_IRQ_TIMEHI	.equ	$F8

; 7 байт состояния каналов.
; Отличается от ACTION тем, что не сбрасывается
; и сохраняет значение до конца воспроизведения ноты
; 0 - канал выключен, 1 - атака, -1 - затухание
CHN1_STATE	.equ	$F9

; Таблицы
TABLES_START	.equ	$4000

ORIG_PITCH_TBL	.equ	TABLES_START + 112 * 128
CHN1_PITCH_TBL	.equ	ORIG_PITCH_TBL + 170
CHN2_PITCH_TBL	.equ	CHN1_PITCH_TBL + 170
CHN3_PITCH_TBL	.equ	CHN2_PITCH_TBL + 170
CHN4_PITCH_TBL	.equ	CHN3_PITCH_TBL + 170
CHN5_PITCH_TBL	.equ	CHN4_PITCH_TBL + 170

SEMITONE_TBL	.equ	CHN5_PITCH_TBL + 170
OCTAVES_TBL	.equ	SEMITONE_TBL + 64
DUR_TBL_INDEX	.equ	OCTAVES_TBL + 7

; Адрес расположения нотного текста
SONG_START	.equ	$8000

; Адреса устройств
KEYBOARD	.equ	$C000
KBDSTROBE	.equ	$C010

; Регистры карты
; первый таймер
TIMER1_CH0	.equ	$C090
TIMER1_CH1	.equ	$C091
TIMER1_CH2	.equ	$C092
TIMER1_CTL	.equ	$C093

; второй таймер
TIMER2_CH0	.equ	$C094
TIMER2_CH1	.equ	$C095
TIMER2_CH2	.equ	$C096
TIMER2_CTL	.equ	$C097

; 5 регистров тональных каналов
CONTROL_CHN1	.equ	$C098
CONTROL_CHN2	.equ	$C099
CONTROL_CHN3	.equ	$C09A
CONTROL_CHN4	.equ	$C09B
CONTROL_CHN5	.equ	$C09C

; 2 регистра ударных каналов
CONTROL_DRUM1	.equ	$C09D
CONTROL_DRUM2	.equ	$C09E

; регистр управления прерываниями
; D7 = 1 запрет прерываний и работы таймеров
; D6 = 1 запрет прерываний от внешнего источника
CONTROL_IRQ	.equ	$C09F

; Адрес вектора маскируемых прерываний
IRQVECTORLO	.equ	$FFFE
IRQVECTORHI	.equ	$FFFF

; Настройки каналов по умолчанию
DEFAULT_TIMBRE	.equ	$15
DEFAULT_TRANSP	.equ	36
DEFAULT_TEMPO	.equ	7
DEFAULT_ATTACK	.equ	3
DEFAULT_DETUNE	.equ	0
ATTACK3_TEMPO7	.equ	TABLES_START + (2 * 16 + 7) * 128

		.org	$E000

;*******************************************************************************
;	Запуск воспроизведения
;*******************************************************************************
		JSR	INIT_CARD
; Копирование указателей на тексты треков
		LDX	#$00

COPY_TRK_PTR_LOOP
		TXA
		ASL	A
		TAY
		LDA	SONG_START + 3, Y
		BNE	TRACK_NOT_EMPTY
; Если старший байт указателя трека нулевой, трек считается пустым
; и ставится флаг "трек остановлен"
		LDA	#$01
		STA	TRACK1_STOPPED, X
		JMP	COPY_TRK_PTR_NEXT

TRACK_NOT_EMPTY
; ?? почему-то для записи в нулевую страницу используется абсолютная адресация
		.assume "addr:absolute"
		STA	TRACK1_PTR + 1, Y
		LDA	SONG_START + 2, Y
		.assume "addr:absolute"
		STA	TRACK1_PTR, Y

COPY_TRK_PTR_NEXT
		INX
		CPX	#$07
		BNE	COPY_TRK_PTR_LOOP

; Обработчик прерывания требует, чтобы все требуемые действия над каналами
; были известны до прихода прерывания. Поэтому надо прочитать первую ноту в
; каждом треке до начала воспроизведения.
		LDX	#$00
		STX	ALL_TRACKS_STOPPED

READ_FIRST_NOTE_LOOP
		LDA	TRACK1_STOPPED, X
		BNE	READ_FIRST_NOTE_NEXT
; Вызов процедуры чтения ноты
		STX	CURRENT_TRACK
		TXA
		ASL	A
		STA	CUR_TRACK_X2
		JSR	GET_NEXT_NOTE_FROM_TRACK
		LDX	CURRENT_TRACK
		LDA	TRACK1_STOPPED, X
		BNE	READ_FIRST_NOTE_NEXT
; Если трек не закончился, перевести канал в фазу атаки
; для воспроизведения новой ноты 
		LDA	#$01
		STA	CHN1_STATE, X
		STA	CHN1_ACTION, X

READ_FIRST_NOTE_NEXT
		INX
		CPX	#$07
		BNE	READ_FIRST_NOTE_LOOP

; Вычислить время следующего прерывания (но в таймер оно не загружается!)
		JSR	CALC_NEXT_IRQ_TIME
; Загрузка в таймер времени первого прерывания.
; Оно фиксированное.
		LDA	#$0A
		STA	TIMER2_CH2
		LDA	#$00
		STA	TIMER2_CH2
		CLI
; Разрешение прерываний на карте
		LDA	#$7F
		STA	CONTROL_IRQ

; Цикл ожидания нажатия на кнопку клавиатуры пока играет музыка
IDLE_LOOP
		LDA	KEYBOARD		
		BPL	IDLE_LOOP

SONG_IS_OVER
		SEI
		STA	KBDSTROBE
		JSR	INIT_CARD
		RTS


;*******************************************************************************
;	Инициализация карты и рабочих переменных
;*******************************************************************************
INIT_CARD
		SEI
; Запрет генерации прерываний	
		LDA	#$FF
		STA	CONTROL_IRQ
; Установка режима счетчика 0 обоих таймеров
; (двоичный счет, делитель - слово, режим генератора прямоугольных импульсов)
		LDA	#$36			
		STA	TIMER1_CTL
		STA	TIMER2_CTL
; Установка режима счетчика 1 обоих таймеров (режим тот же)
		LDA	#$76
		STA	TIMER1_CTL
		STA	TIMER2_CTL
; Установка режима счетчика 2 таймера 1 (режим тот же)
		LDA	#$B6
		STA	TIMER1_CTL
; Установка режима счетчика 2 таймера 2 (генератора прерываний)
; (двоичный счет, делитель - слово, режим программного строба)
		LDA	#$B8			
		STA	TIMER2_CTL

; Глушение всех каналов
		LDX	#$06
		LDA	#$00

MUTE_CARD_LOOP
		STA	CONTROL_CHN1, X
		DEX
		BPL	MUTE_CARD_LOOP

; Очистка рабочей области
		LDX	#$00			

CLEAR_WORK_AREA_LOOP
		STA	SONG_EVENTS, X
		INX	
		CPX	#<(WORK_AREA_END - SONG_EVENTS)
		BNE	CLEAR_WORK_AREA_LOOP

; Очистка переменных в нулевой странице
; Область $C5 - $CF
		LDX	#DETUNE_HI

CLEAR_ZEROPAGE1
		STA	ZEROPAGE_START, X
		INX
		CPX	#CUR_TRACK_X2 + 1
		BNE	CLEAR_ZEROPAGE1

; Область $D8 - $FF
		LDX	#CHN1_ACTION

CLEAR_ZEROPAGE2
		STA	ZEROPAGE_START, X
		INX
		BNE	CLEAR_ZEROPAGE2

; Для мелодических каналов установить тембр и октаву (3) по умолчанию
		LDX	#$00

MELODIC_DEFAULTS_LOOP
		LDA	#DEFAULT_TIMBRE
		STA	CHN1_TIMBRE, X
		LDA	#DEFAULT_TRANSP
		STA	CHN1_TRANSPOSE, X
		INX
		CPX	#$05
		BNE	MELODIC_DEFAULTS_LOOP

; Для всех каналов установить темп (7) и атаку (3) по умолчанию
		LDX	#$00

CHANNEL_DEFAULTS_LOOP
		LDA	#DEFAULT_TEMPO
		STA	CHN1_TEMPO, X
		LDA	#DEFAULT_ATTACK
		STA	CHN1_ATTACK_VALUE, X
		INX
		CPX	#$07
		BNE	CHANNEL_DEFAULTS_LOOP

; Сами по себе значения темпа и атаки не работают, если не
; установить правильную таблицу длительностей
		LDX	#$00

SET_DURATION_TABLE_LOOP
		LDA	#<ATTACK3_TEMPO7
		STA	CHN1_DURATTBL_PTR, X
		INX
		LDA	#>ATTACK3_TEMPO7
		STA	CHN1_DURATTBL_PTR, X
		INX
		CPX	#$0E
		BMI	SET_DURATION_TABLE_LOOP

; Установка таблиц высоты нот
		LDA	#<CHN1_PITCH_TBL
		STA	CHN1_PITCHTBL_PTR
		LDA	#>CHN1_PITCH_TBL
		STA	CHN1_PITCHTBL_PTR + 1
		LDA	#<CHN2_PITCH_TBL
		STA	CHN2_PITCHTBL_PTR
		LDA	#>CHN2_PITCH_TBL
		STA	CHN2_PITCHTBL_PTR + 1
		LDA	#<CHN3_PITCH_TBL
		STA	CHN3_PITCHTBL_PTR
		LDA	#>CHN3_PITCH_TBL
		STA	CHN3_PITCHTBL_PTR + 1
		LDA	#<CHN4_PITCH_TBL
		STA	CHN4_PITCHTBL_PTR
		LDA	#>CHN4_PITCH_TBL
		STA	CHN4_PITCHTBL_PTR + 1
		LDA	#<CHN5_PITCH_TBL
		STA	CHN5_PITCHTBL_PTR
		LDA	#>CHN5_PITCH_TBL
		STA	CHN5_PITCHTBL_PTR + 1

; Задание расстройки для всех мелодических каналов
		LDX	#$00
		LDA	PITCH_TBL_PTR
		PHA
		LDA	PITCH_TBL_PTR + 1
		PHA

SET_DETUNE_LOOP
		LDA	CHN1_PITCHTBL_PTR, X
		STA	PITCH_TBL_PTR
		INX
		LDA	CHN1_PITCHTBL_PTR, X
		STA	PITCH_TBL_PTR + 1
		INX
		TXA
		PHA
		LDA	#DEFAULT_DETUNE
		JSR	SET_CHANNEL_DETUNE
		PLA
		TAX
		CPX	#$0A
		BMI	SET_DETUNE_LOOP
		PLA
		STA	PITCH_TBL_PTR + 1
		PLA
		STA	PITCH_TBL_PTR

; Установка обработчика прерываний
		LDA	#<IRQ_HANDLER
		STA	IRQVECTORLO
		LDA	#>IRQ_HANDLER
		STA	IRQVECTORHI
		RTS


;*******************************************************************************
;	Обработчик прерываний
; Воспроизведение звука делится на 2 части: атаку и затухание.
; Управление воспроизведением в канале выполняется через переменную CHNx_ACTION
; Если она равна 1, в таймер грузится делитель частоты из ячеек CHNx_PITCHLO,
; CHNx_PITCHHI, а в управляющий регистр - тембр из ячейки CHNx_ATTACK
; Если она равна -1, частота не меняется, а в управляющий регистр грузится
; тембр из ячейки CHNx_DECAY (он не имеет битов громкости). Нулевое значение
; используется как признак легато, при этом нота продолжает играть.
;*******************************************************************************
IRQ_HANDLER
		PHA
		TXA
		PHA

; Функция воспроизведения от момента установки события с указанным номером.
; Если значение переменной SKIP_TO_EVENT не 0, то ничего не играется, а нотный
; текст читается до момента выполнения команды установки события.
; Все команды при этом выполняются, но таймер не устанавливается.
		LDA	SKIP_TO_EVENT
		BEQ	CHANNEL1_CHECK
		JMP	DO_SKIP

CHANNEL1_CHECK
		LDA	CHN1_ACTION
		BEQ	CHANNEL2_CHECK
		BMI	SET_CHN1_DECAY
; новая нота, загрузка счетчика таймера
		LDA	CHN1_PITCHLO
		STA	TIMER1_CH0
		LDA	CHN1_PITCHHI
		STA	TIMER1_CH0
; запись тембра в управляющий регистр
		LDA	CHN1_ATTACK
		STA	CONTROL_CHN1
		JMP	CHANNEL2_CHECK

SET_CHN1_DECAY
; для фазы затухания используется второй тембр (без битов громкости)
		LDA	CHN1_DECAY
; если второй тембр равен 0, то нота не выключается
; за счет этого реализуется легато
		BEQ	CHANNEL2_CHECK
		STA	CONTROL_CHN1

CHANNEL2_CHECK
		LDA	CHN2_ACTION
		BEQ	CHANNEL3_CHECK
		BMI	SET_CHN2_DECAY
		LDA	CHN2_PITCHLO
		STA	TIMER1_CH1
		LDA	CHN2_PITCHHI
		STA	TIMER1_CH1
		LDA	CHN2_ATTACK
		STA	CONTROL_CHN2
		JMP	CHANNEL3_CHECK

SET_CHN2_DECAY
		LDA	CHN2_DECAY
		BEQ	CHANNEL3_CHECK
		STA	CONTROL_CHN2

CHANNEL3_CHECK
		LDA	CHN3_ACTION
		BEQ	CHANNEL4_CHECK
		BMI	SET_CHN3_DECAY
		LDA	CHN3_PITCHLO
		STA	TIMER1_CH2
		LDA	CHN3_PITCHHI
		STA	TIMER1_CH2
		LDA	CHN3_ATTACK
		STA	CONTROL_CHN3
		JMP	CHANNEL4_CHECK

SET_CHN3_DECAY
		LDA	CHN3_DECAY
		BEQ	CHANNEL4_CHECK
		STA	CONTROL_CHN3

CHANNEL4_CHECK
		LDA	CHN4_ACTION
		BEQ	CHANNEL5_CHECK
		BMI	SET_CHN4_DECAY
		LDA	CHN4_PITCHLO
		STA	TIMER2_CH0
		LDA	CHN4_PITCHHI
		STA	TIMER2_CH0
		LDA	CHN4_ATTACK
		STA	CONTROL_CHN4
		JMP	CHANNEL5_CHECK

SET_CHN4_DECAY
		LDA	CHN4_DECAY
		BEQ	CHANNEL5_CHECK
		STA	CONTROL_CHN4

CHANNEL5_CHECK
		LDA	CHN5_ACTION
		BEQ	DRUM1_CHECK
		BMI	SET_CHN5_DECAY
		LDA	CHN5_PITCHLO
		STA	TIMER2_CH1
		LDA	CHN5_PITCHHI
		STA	TIMER2_CH1
		LDA	CHN5_ATTACK
		STA	CONTROL_CHN5
		JMP	DRUM1_CHECK

SET_CHN5_DECAY
		LDA	CHN5_DECAY
		BEQ	DRUM1_CHECK
		STA	CONTROL_CHN5

DRUM1_CHECK
		LDA	DRUM1_ACTION
		BEQ	DRUM2_CHECK
		BMI	SET_DRUM1_DECAY
		LDA	DRUM1_ATTACK
		STA	CONTROL_DRUM1
		JMP	DRUM2_CHECK

SET_DRUM1_DECAY
		LDA	DRUM1_DECAY
		STA	CONTROL_DRUM1

DRUM2_CHECK
		LDA	DRUM2_ACTION
		BEQ	RELOAD_INT_TIMER
		BMI	SET_DRUM2_DECAY
		LDA	DRUM2_ATTACK
		STA	CONTROL_DRUM2
		JMP	RELOAD_INT_TIMER

SET_DRUM2_DECAY
		LDA	DRUM2_DECAY
		STA	CONTROL_DRUM2

RELOAD_INT_TIMER
; перезагрузка таймера прерываний ранее рассчитанным значением
; для генерации следующего прерывания
		LDA	NEXT_IRQ_TIMELO
		STA	TIMER2_CH2
		LDA	NEXT_IRQ_TIMEHI
		STA	TIMER2_CH2

DO_SKIP
; обнуление действий для всех каналов
		LDX	#$06
		LDA	#$00

RESET_CHN_ACTIONS
		STA	CHN1_ACTION, X
		DEX
		BPL	RESET_CHN_ACTIONS
		LDX	#$00

; Цикл обработки команд в треках
TRACK_COMMAND_LOOP
; Завершившиеся треки пропускаются
		LDA	TRACK1_STOPPED, X
		BNE	COMMAND_LOOP_NEXT
		LDA	CHN1_STATE, X
		BMI	DECREASE_DECAY_TIME
; Канал в состоянии атаки
; Уменьшаем время атаки на длительность прерывания
; (то время, на которое ставился таймер перед возникновением
; этого прерывания)
		LDA	CHN1_ATTACK_TIME_LO, X
		SEC
		SBC	NEXT_IRQ_TIMELO
		STA	CHN1_ATTACK_TIME_LO, X
		LDA	CHN1_ATTACK_TIME_HI, X
		SBC	NEXT_IRQ_TIMEHI
		STA	CHN1_ATTACK_TIME_HI, X
		BNE	COMMAND_LOOP_NEXT
		LDA	CHN1_ATTACK_TIME_LO, X
		BNE	COMMAND_LOOP_NEXT
; Время уменьшилось до 0, значит фаза атаки закончилась
; переводим канал в фазу затухания
		LDA	#$FF
		STA	CHN1_STATE, X
		STA	CHN1_ACTION, X
		BMI	COMMAND_LOOP_NEXT

DECREASE_DECAY_TIME
; Канал в состоянии затухания
; Уменьшаем время затухания на длительность прерывания
		LDA	CHN1_DECAY_TIME_LO, X
		SEC
		SBC	NEXT_IRQ_TIMELO
		STA	CHN1_DECAY_TIME_LO, X
		LDA	CHN1_DECAY_TIME_HI, X
		SBC	NEXT_IRQ_TIMEHI
		STA	CHN1_DECAY_TIME_HI, X
		BNE	COMMAND_LOOP_NEXT
		LDA	CHN1_DECAY_TIME_LO, X
		BNE	COMMAND_LOOP_NEXT
; Время уменьшилось до 0, значит воспроизведение ноты закончилось.
; Необходимо получить следующую из текущего трека
		TYA
		PHA
		STX	CURRENT_TRACK
		TXA
		ASL	A
		STA	CUR_TRACK_X2
		JSR	GET_NEXT_NOTE_FROM_TRACK
		LDX	CURRENT_TRACK
		PLA
		TAY
		LDA	TRACK1_STOPPED, X
		BNE	COMMAND_LOOP_NEXT
; Если при чтении очередной ноты не оказалось, что трек закончился,
; перевести канал в фазу атаки для воспроизведения новой ноты 
		LDA	#$01
		STA	CHN1_STATE, X
		STA	CHN1_ACTION, X

COMMAND_LOOP_NEXT
		INX
		CPX	#$07
		BNE	TRACK_COMMAND_LOOP

; Вычислить время следующего прерывания
		JSR	CALC_NEXT_IRQ_TIME

; Цикл проверки окончания воспроизведения
		LDX	#$00

CHECK_SONG_IS_OVER_LOOP
		LDA	TRACK1_STOPPED, X
		BEQ	SKIP_TO_EVENT_CHECK
		INX
		CPX	#$07
		BNE	CHECK_SONG_IS_OVER_LOOP
; Если пришли сюда, значит все каналы остановлены.
; Устанавливаем флаг, что все каналы остановлены и
; решительно выходим из прерывания в код плеера.
; Зачем было так делать, если в плеере уже есть
; опрос клавиатуры? Можно было добавить туда же
; опрос и этого флага.
		INC	ALL_TRACKS_STOPPED
		PLA
		PLA
		TSX
		INX
		INX
		INX
		TXS
		JMP	SONG_IS_OVER

EXIT_IRQ_HANDLER
		PLA
		TAX
		PLA
		CLI
		RTI

SKIP_TO_EVENT_CHECK
; Функция воспроизведения от момента установки события с указанным номером.
; Если значение переменной SKIP_TO_EVENT равно 0, то выходим из прерывания.
		LDX	SKIP_TO_EVENT
		BEQ	EXIT_IRQ_HANDLER
; Проверяем событие с номером, указанным в SKIP_TO_EVENT
		LDA	SONG_EVENTS, X
		BEQ	CONTINUE_READING
; Событие установлено. Очищаем SKIP_TO_EVENT и выходим из прерывания
; ?? Но видимо тут ошибка - пропущена загрузка счетчика таймера,
; поэтому новое прерывание не возникнет.
		LDA	#$00
		STA	SKIP_TO_EVENT
		BEQ	EXIT_IRQ_HANDLER		; переход всегда выполняется

		LDA	ALL_TRACKS_STOPPED		; ?? сюда вообще входа нет
		BNE	EXIT_IRQ_HANDLER

CONTINUE_READING
; Событие не установлено, продолжаем читать нотный текст
		JMP	DO_SKIP


;*******************************************************************************
; 	Получение следующей ноты из трека
; Если в тексте присутствует команда, она выполняется и чтение продолжается,
; пока не будет найдена нота, пауза или признак конца трека.
; Если найден признак конца трека, ставится флаг "Трек остановлен"
; На входе:
;	CUR_TRACK_X2 - номер трека x 2
;	CURRENT_TRACK - номер трека
;*******************************************************************************
GET_NEXT_NOTE_FROM_TRACK

; Загрузка указателей на таблицы длительностей и высот данного канала
		LDX	CUR_TRACK_X2
		LDA	CHN1_PITCHTBL_PTR, X
		STA	PITCH_TBL_PTR
		LDA	CHN1_PITCHTBL_PTR + 1, X
		STA	PITCH_TBL_PTR + 1
		LDA	CHN1_DURATTBL_PTR, X
		STA	DURAT_TBL_PTR
		LDA	CHN1_DURATTBL_PTR + 1, X
		STA	DURAT_TBL_PTR + 1

GET_NEXT_NOTE2
		LDA	(TRACK1_PTR, X)

; Положительное значение - это нота или пауза
		BPL	NOTE_OR_PAUSE
		JMP	IS_TEMPO_CMD

NOTE_OR_PAUSE
		BNE	READ_NOTE

; Прочитан ноль - это пауза
		LDX	CURRENT_TRACK
		CPX	#$05
		BMI	MELODIC_PAUSE

; Для ударных каналов тембры ставятся по нулям
		LDA	#$00
		STA	CHN1_ATTACK, X
		STA	CHN1_DECAY, X
		JMP	SET_DURATION

; Для мелодических каналов из тембра берутся настройки фильтров,
; чтобы ранее начавшиеся затухающие ноты не меняли тембр
MELODIC_PAUSE
		LDA	CHN1_TIMBRE, X
		AND	#$0F
		STA	CHN1_ATTACK, X
		STA	CHN1_DECAY, X
		JMP	SET_DURATION

; Новая нота
READ_NOTE
		LDX	CURRENT_TRACK
		CPX	#$05
		BMI	MELODIC_NOTE

; Для ударных каналов 1-й байт содержит тембр
; Бит 4 включается принудительно
		ORA	#$10
		STA	CHN1_ATTACK, X
		AND	#$0F
		STA	CHN1_DECAY, X
		JMP	SET_DURATION

; Для мелодических каналов загружается новый делитель частоты
MELODIC_NOTE
		TAY
; Номер ноты перекодируется в относительный номер полутона	
		LDA	SEMITONE_TBL, Y
		CLC
; и складывается со значением транспозиции канала
		ADC	CHN1_TRANSPOSE, X
		ASL	A
		TAY
; По полученному абсолютному номеру полутона получается значение делителя
		LDA	(PITCH_TBL_PTR), Y
		STA	CHN1_PITCHLO, X
		INY	
		LDA	(PITCH_TBL_PTR), Y
		STA	CHN1_PITCHHI, X
; Тембры атаки и затухания формируются из тембра канала
		LDA	CHN1_TIMBRE, X
		STA	CHN1_ATTACK, X
		AND	#$0F
		STA	CHN1_DECAY, X

SET_DURATION
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDX	CURRENT_TRACK
; Умножение на 4
		ASL	A
		ASL	A
; Бит D6 (лига) попадает в флаг C
		BCC	NO_SLUR

; Если есть лига, тембр затухания устанавливается в 0
; Обработчик прерывания при этом не выключает ноту (кроме ударных каналов)
		PHA
		LDA	#$00
		STA	CHN1_DECAY, X
		PLA

NO_SLUR
; Копирование 2 байт времени атаки и 2 байт времени затухания
		TAY
		LDA	(DURAT_TBL_PTR), Y
		STA	CHN1_ATTACK_TIME_LO, X
		INY
		LDA	(DURAT_TBL_PTR), Y
		STA	CHN1_ATTACK_TIME_HI, X
		INY
		LDA	(DURAT_TBL_PTR), Y
		STA	CHN1_DECAY_TIME_LO, X
		INY
		LDA	(DURAT_TBL_PTR), Y
		STA	CHN1_DECAY_TIME_HI, X
		JSR	INC_TRACK_PTR
		RTS

; Команды
IS_TEMPO_CMD
		CMP	#$D0			; установка темпа
		BNE	IS_TRANSPOSE_CMD
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDX	CURRENT_TRACK
		STA	CHN1_TEMPO, X

SET_TEMPO_AND_ATTACK
; Номер требуемой таблицы длительностей определяется
; как значение_атаки * 16 + значение_темпа
		LDA	CHN1_ATTACK_VALUE, X
		ASL	A
		ASL	A
		ASL	A
		ASL	A
		CLC
		ADC	CHN1_TEMPO, X
; Номер таблицы умножаем на 2 и берем адрес из таблицы
; указателей на таблицы длительностей
		ASL	A
		TAY
		LDX	CUR_TRACK_X2
		LDA	DUR_TBL_INDEX, Y
		STA	CHN1_DURATTBL_PTR, X
		STA	DURAT_TBL_PTR
		LDA	DUR_TBL_INDEX + 1, Y
		STA	CHN1_DURATTBL_PTR + 1, X
		STA	DURAT_TBL_PTR + 1
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_TRANSPOSE_CMD
		CMP	#$D2			; установка транспозиции
		BNE	IS_TIMBRE_CMD
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDX	CURRENT_TRACK
; Значение в команде суммируется с текущим
		CLC
		ADC	CHN1_TRANSPOSE, X
		STA	CHN1_TRANSPOSE, X
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_TIMBRE_CMD
		CMP	#$D5			; установка тембра
		BNE	IS_OCTAVE_UP_CMD
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDX	CURRENT_TRACK
		STA	CHN1_TIMBRE, X
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_OCTAVE_UP_CMD
		CMP	#$D6			; транспозиция на октаву вверх
		BNE	IS_OCTAVE_DOWN_CMD	; (октава - это 12 полутонов)
		LDA	#12
		LDX	CURRENT_TRACK
		CLC
		ADC	CHN1_TRANSPOSE, X
		STA	CHN1_TRANSPOSE, X
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_OCTAVE_DOWN_CMD
		CMP	#$D7			; транспозиция на октаву вниз
		BNE	IS_END_OF_TRK_CMD
		LDX	CURRENT_TRACK
		LDA	CHN1_TRANSPOSE, X
		SEC
		SBC	#12
		STA	CHN1_TRANSPOSE, X
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_END_OF_TRK_CMD
		CMP	#$D1			; конец трека
		BNE	IS_DETUNE_CMD
		LDX	CURRENT_TRACK
; Установить флаг остановки трека
		LDA	#$01
		STA	TRACK1_STOPPED, X
		RTS

IS_DETUNE_CMD
		CMP	#$E0			; расстройка
		BMI	IS_INTERRUPT_CMD
		CMP	#$F0
		BPL	IS_ATTACK_CMD
		SEC
		SBC	#$E0
		JSR	SET_CHANNEL_DETUNE
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_ATTACK_CMD
		CMP	#$F8			; установка атаки
		BPL	IS_OCTAVE_CMD		; значение $F7 недопустимо!
		LDX	CURRENT_TRACK
		SEC
		SBC	#$F0
		STA	CHN1_ATTACK_VALUE, X
		JMP	SET_TEMPO_AND_ATTACK

IS_OCTAVE_CMD
		SEC				; установка октавы
		SBC	#$F8
		TAY
; Перекодировка номера октавы в число полутонов
; ?? Формально тут ошибка - в таблице 7 значений и при значении $FF будет выход
; за пределы массива. Но, поскольку после таблицы идет список указателей на
; таблицы длительностей, а первый байт там 0, то установка 7-й октавы сработает
; как установка 0-й.
		LDA	OCTAVES_TBL, Y
		LDX	CURRENT_TRACK
		STA	CHN1_TRANSPOSE, X
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_INTERRUPT_CMD
		CMP	#$D4			; прерывание канала
		BNE	IS_LOOP_END_CMD

SET_TRACK_POINTER
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		STA	TEMP_VAL
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		STA	TRACK1_PTR + 1, X
		LDA	TEMP_VAL
		STA	TRACK1_PTR, X
		JMP	GET_NEXT_NOTE2

IS_LOOP_END_CMD
		CMP	#$DA			; переход к началу цикла
		BEQ	SET_TRACK_POINTER
		CMP	#$D8			; начало цикла
		BNE	IS_SET_EVENT_CMD
		JSR	INC_TRACK_PTR
; Первый байт параметров - это число повторов
		LDA	(TRACK1_PTR, X)
		BNE	INC_LOOP_COUNTER
		JSR	INC_TRACK_PTR
		JMP	SKIP_LOOP_PARAMS

INC_LOOP_COUNTER
		STA	TEMP_VAL
		INC	TEMP_VAL
		JSR	INC_TRACK_PTR
; Второй байт параметров - счётчик цикла
; В исходном нотном тексте должен быть равен 0
		LDA	(TRACK1_PTR, X)
		CLC
		ADC	#$01
		STA	(TRACK1_PTR, X)
		LDX	CURRENT_TRACK
; Копия счётчика цикла помещается в переменную CHNx_LOOP_COUNTER_VAL,
; для использования в обработке событий
		STA	CHN1_LOOP_COUNTER_VAL, X
		LDX	CUR_TRACK_X2
		CMP	TEMP_VAL
		BMI	SKIP_LOOP_PARAMS
; Если достигнуто нужное число повторов, второй байт параметров обнуляется
		LDA	#$00
		STA	(TRACK1_PTR, X)
		JMP	SET_TRACK_POINTER

SKIP_LOOP_PARAMS
; Пропуск параметров команды цикла
		JSR	INC_TRACK_PTR
		JSR	INC_TRACK_PTR
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_SET_EVENT_CMD
		CMP	#$DB			; установка события
		BNE	IS_RESET_EVENT_CMD
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		TAY
		LDA	#$01
		STA	SONG_EVENTS, Y
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_RESET_EVENT_CMD
		CMP	#$DC			; сброс события
		BNE	IS_JUMP_UNC_CMD
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		TAY
		LDA	#$00
		STA	SONG_EVENTS, Y
		JSR	INC_TRACK_PTR
		JMP	GET_NEXT_NOTE2

IS_JUMP_UNC_CMD
		CMP	#$DD			; безусловный переход
		BNE	IS_JUMP_TRK_CMD
		JSR	INC_TRACK_PTR		; пропустить байт параметра
		JMP	SET_TRACK_POINTER

IS_JUMP_TRK_CMD
		CMP	#$C1			; переход при несовпадении параметра
		BNE	IS_JUMP_LOOP_CMD	; с номером текущего трека
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDY	CURRENT_TRACK
		INY
		STY	TEMP_VAL
		CMP	TEMP_VAL
		BEQ	SKIP_LOOP_PARAMS
		JMP	SET_TRACK_POINTER

IS_JUMP_LOOP_CMD
		CMP	#$C2			; переход при несовпадении параметра
		BNE	IS_JUMP_EVENT_CMD	; с счётчиком цикла
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		LDY	CURRENT_TRACK
		CMP	CHN1_LOOP_COUNTER_VAL, Y
		BEQ	SKIP_LOOP_PARAMS
		JMP	SET_TRACK_POINTER

IS_JUMP_EVENT_CMD
		CMP	#$C3			; переход, если указанное в параметре
		BNE	UNKNOWN_CMD		; событие не установлено
		JSR	INC_TRACK_PTR
		LDA	(TRACK1_PTR, X)
		TAY
		LDA	SONG_EVENTS, Y
		BNE	SKIP_LOOP_PARAMS
		JMP	SET_TRACK_POINTER

UNKNOWN_CMD
		JSR	INC_TRACK_PTR		; неизвестная команда
		JMP	GET_NEXT_NOTE2		; Считается, что у нее 1 параметр


;*******************************************************************************
; 	Инкремент указателя текста трека
;*******************************************************************************
INC_TRACK_PTR
		LDX	CUR_TRACK_X2
		INC	TRACK1_PTR, X
		BNE	INC_TRACK_EXIT
		INC	TRACK1_PTR + 1, X

INC_TRACK_EXIT
		RTS


;*******************************************************************************
; 	Задание расстройки высоты нот в канале
; Производится путем построения новой таблицы высот для канала.
; На входе:
;	A - значение расстройки (0 - F)
;	PITCH_TBL_PTR - указатель на таблицу высот канала
;
; В самой верхней октаве значение расстройки прибавляется к коэффициенту
; деления для каждой ноты. При переходе на октаву ниже значение расстройки
; удваивается.
;*******************************************************************************
SET_CHANNEL_DETUNE
		STA	DETUNE_LO
		LDA	#$00
		STA	DETUNE_HI
		LDY	#169			; 85 делителей x 2 байта - 1

DETUNE_LOOP
		DEY
		LDA	ORIG_PITCH_TBL, Y
		CLC
		ADC	DETUNE_LO
		STA	(PITCH_TBL_PTR), Y
		INY
		LDA	ORIG_PITCH_TBL, Y
		ADC	DETUNE_HI
		STA	(PITCH_TBL_PTR), Y
		DEY
		DEY
		CPY	#$FF
		BEQ	DETUNE_EXIT
; Определение начала следующей октавы
		CPY	#1
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#25
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#49
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#73
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#97
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#121
		BEQ	DOUBLE_DETUNE_VALUE
		CPY	#145
		BEQ	DOUBLE_DETUNE_VALUE
		JMP	DETUNE_LOOP

DOUBLE_DETUNE_VALUE				; для следующей октавы удвоить
		CLC	
		ASL	DETUNE_LO
		ROL	DETUNE_HI
		JMP	DETUNE_LOOP

DETUNE_EXIT
		RTS


;*******************************************************************************
; 	Расчет времени следующего срабатывания таймера прерываний
; Для каждого канала, трек которого не остановлен, берется, в зависимости от
; его состояния, время атаки или время затухания и ищется минимум.
;*******************************************************************************
CALC_NEXT_IRQ_TIME
		LDA	#$FF
		STA	NEXT_IRQ_TIMEHI
		STA	NEXT_IRQ_TIMELO
		LDX	#$00

MIN_TIME_LOOP
		LDA	TRACK1_STOPPED, X
		BEQ	FIND_MIN_TIME
		JMP	MIN_TIME_LOOP_NEXT

FIND_MIN_TIME
		LDA	NEXT_IRQ_TIMEHI
		BMI	MIN_TIMEHI_NOT_SET
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME1
		LDA	CHN1_ATTACK_TIME_HI, X
		BPL	CMP_TIME_HIBYTE
		BMI	MIN_TIME_LOOP_NEXT

USE_DECAY_TIME1
		LDA	CHN1_DECAY_TIME_HI, X
		BMI	MIN_TIME_LOOP_NEXT

CMP_TIME_HIBYTE
		CMP	NEXT_IRQ_TIMEHI
		BMI	SET_NEW_MIN_TIME
		BEQ	TIMEHI_EQUAL
		BPL	MIN_TIME_LOOP_NEXT

MIN_TIMEHI_NOT_SET
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME2
		LDA	CHN1_ATTACK_TIME_HI, X
		BPL	SET_NEW_MIN_TIME
		BMI	CMP_TIME_HIBYTE2

USE_DECAY_TIME2
		LDA	CHN1_DECAY_TIME_HI, X
		BPL	SET_NEW_MIN_TIME

CMP_TIME_HIBYTE2
		CMP	NEXT_IRQ_TIMEHI
		BMI	SET_NEW_MIN_TIME
		BEQ	TIMEHI_EQUAL
		BPL	MIN_TIME_LOOP_NEXT

TIMEHI_EQUAL
		LDA	NEXT_IRQ_TIMELO
		BMI	MIN_TIMELO_NOT_SET
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME3
		LDA	CHN1_ATTACK_TIME_LO, X
		BMI	MIN_TIME_LOOP_NEXT
		BPL	CMP_TIME_LOBYTE

USE_DECAY_TIME3
		LDA	CHN1_DECAY_TIME_LO, X
		BMI	MIN_TIME_LOOP_NEXT

CMP_TIME_LOBYTE
		CMP	NEXT_IRQ_TIMELO
		BMI	SET_NEW_MIN_TIME
		BPL	MIN_TIME_LOOP_NEXT

MIN_TIMELO_NOT_SET
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME4
		LDA	CHN1_ATTACK_TIME_LO, X
		BPL	SET_NEW_MIN_TIME
		BMI	CMP_TIME_LOBYTE2

USE_DECAY_TIME4
		LDA	CHN1_DECAY_TIME_LO, X
		BPL	SET_NEW_MIN_TIME

CMP_TIME_LOBYTE2
		CMP	NEXT_IRQ_TIMELO
		BPL	MIN_TIME_LOOP_NEXT

SET_NEW_MIN_TIME
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME5
		LDA	CHN1_ATTACK_TIME_HI, X
		JMP	SET_NEW_MIN_TIMEHI

USE_DECAY_TIME5
		LDA	CHN1_DECAY_TIME_HI, X

SET_NEW_MIN_TIMEHI
		STA	NEXT_IRQ_TIMEHI
		LDA	CHN1_STATE, X
		BMI	USE_DECAY_TIME6
		LDA	CHN1_ATTACK_TIME_LO, X
		STA	NEXT_IRQ_TIMELO
		JMP	MIN_TIME_LOOP_NEXT

USE_DECAY_TIME6
		LDA	CHN1_DECAY_TIME_LO, X
		STA	NEXT_IRQ_TIMELO

MIN_TIME_LOOP_NEXT
		INX
		CPX	#$07
		BEQ	CALC_NEXT_IRQ_TIME_END
		JMP	MIN_TIME_LOOP

CALC_NEXT_IRQ_TIME_END
		RTS


; Тембры для каждого мелодического канала
CHN1_TIMBRE
		.fill	5

; Транспонирование для каждого мелодического канала
CHN1_TRANSPOSE
		.fill	5

; Указатели на таблицы длительности для каждого канала
CHN1_DURATTBL_PTR
		.word	$00, $00, $00, $00, $00, $00, $00

; Указатели на таблицы высот для каждого мелодического канала
CHN1_PITCHTBL_PTR
		.word	$00

CHN2_PITCHTBL_PTR
		.word	$00

CHN3_PITCHTBL_PTR
		.word	$00

CHN4_PITCHTBL_PTR
		.word	$00

CHN5_PITCHTBL_PTR
		.word	$00

; Темпы каналов
CHN1_TEMPO
		.fill	7

; Значения атаки каналов
CHN1_ATTACK_VALUE
		.fill	7

; События
SONG_EVENTS
		.fill	33

; Копии переменных счетчиков цикла
CHN1_LOOP_COUNTER_VAL
		.fill	7

; Младшие байты делителей частоты
CHN1_PITCHLO
		.byte	$00

CHN2_PITCHLO
		.byte	$00

CHN3_PITCHLO
		.byte	$00

CHN4_PITCHLO
		.byte	$00

CHN5_PITCHLO
		.byte	$00

; Старшие байты делителей частоты
CHN1_PITCHHI
		.byte	$00

CHN2_PITCHHI
		.byte	$00

CHN3_PITCHHI
		.byte	$00

CHN4_PITCHHI
		.byte	$00

CHN5_PITCHHI
		.byte	$00

; Тембры атаки
CHN1_ATTACK
		.byte	$00

CHN2_ATTACK
		.byte	$00

CHN3_ATTACK
		.byte	$00

CHN4_ATTACK
		.byte	$00

CHN5_ATTACK
		.byte	$00

DRUM1_ATTACK
		.byte	$00

DRUM2_ATTACK
		.byte	$00

; Счетчики времени атаки ноты.
; Каждое прерывание уменьшаются на число, записанное в таймер
CHN1_ATTACK_TIME_LO
		.fill	7

; Старшие байты счетчиков
CHN1_ATTACK_TIME_HI
		.fill	7

; Тембры затухания
CHN1_DECAY
		.byte	$00

CHN2_DECAY
		.byte	$00

CHN3_DECAY
		.byte	$00

CHN4_DECAY
		.byte	$00

CHN5_DECAY
		.byte	$00

DRUM1_DECAY
		.byte	$00

DRUM2_DECAY
		.byte	$00

; Счетчики времени затухания ноты.
; Каждое прерывание уменьшаются на число, записанное в таймер
CHN1_DECAY_TIME_LO
		.fill	7

; Старшие байты счетчиков
CHN1_DECAY_TIME_HI
		.fill	7

; Признак, что все треки закончились
ALL_TRACKS_STOPPED
		.byte	$00

WORK_AREA_END

; Это просто неинициализированная память.
; Оставлена как есть, чтобы проще было сравнивать
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $02, $00, $00, $00, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
		.byte	$FF, $FF, $FF, $FF, $FF, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $00, $00, $00, $00


;*******************************************************************************
;	Создание таблиц
;
; Размещение и структура таблиц:
;	$4000 - $77FF	массив таблиц длительностей (112 элементов по 128 байт)
;	$7800 - $7BFB	массив таблиц высот нот (6 таблиц по 170 байт)
;	$7BFC - $7C3B	таблица перекодировки ноты в номер полутона (64 байта)
;	$7C3C - $7C42	таблица перекодировки октавы в номер полутона (7 байт)
;	$7C43 - $7D22	таблица указателей на элементы массива таблиц
;			длительностей (224 байта)
;
; Массив таблиц длительностей - это двумерный массив [7][16]. Каждый элемент
; массива - это таблица, рассчитанная для указанных значений атаки (0-6) и
; темпа (0-15). Каждая таблица содержит 4 части по 32 байта. Каждая часть
; соответствует типу ноты (0 - обычная, 1 - триольная, 2 - с точкой,
; 3 - триольная с точкой) и содержит 7 пар двухбайтных значений атаки и
; затухания для каждой длительности ноты (от целой до 1/64, всего 28 байт)
; и 4 нулевых байта. Длительность ноты получается как сумма времени атаки и
; затухания. Значения заданы в тактах таймера.
; 
; Массив таблиц высот нот содержит основную таблицу по адресу $7800 и
; 5 ее копий для каждого мелодического канала. При выполнении команды
; "расстройка" таблица указанного канала модифицируется. Таблица содержит
; 85 значений делителей частоты для нот от си субсубконтроктавы до си 3-й
; октавы.
;
; Таблица перекодировки ноты в номер полутона содержит 4 части по 16 байт.
; Каждая часть соответствует знаку при ноте (0 - знака нет, 1 - диез,
; 3 - бемоль). Часть 2 не используется.
;*******************************************************************************
TABLES_PTRLO	.equ	$08
TABLES_PTRHI	.equ	$09
SAVE_Y		.equ	$0A
ACC_NORMAL_LO	.equ	$F1
ACC_NORMAL_HI	.equ	$F2
ACC_DOTTED_LO	.equ	$F3
ACC_DOTTED_HI	.equ	$F4
ACC_TRIPLET_LO	.equ	$F5
ACC_TRIPLET_HI	.equ	$F6
BASE_DUR_LO	.equ	$F5
BASE_DUR_HI	.equ	$F6
TEMP_DUR_LO	.equ	$F7
TEMP_DUR_HI	.equ	$F8
DUR_INDEX	.equ	$F9

ZP_A4		.equ	$A4		; В эти ячейки заносятся значения,
ZP_A5		.equ	$A5		; но драйвер их не использует.
ZP_D0		.equ	$D0		; Часть ячеек им затирается
ZP_D1		.equ	$D1		; и используется с другими целями.
ZP_DC		.equ	$DC		; Возможно, это отладочный вывод
ZP_DD		.equ	$DD
ZP_F0		.equ	$F0

		.org	$E700

INIT_TABLES
		LDA	#$00
		STA	TEMPO_CNTR
		STA	TABLES_PTRLO
		STA	ATTACK_CNTR
		STA	DURATION_CNTR
		STA	NOTE_TYPE_CNTR
		LDA	#>TABLES_START
		STA	TABLES_PTRHI

; for (ATTACK_CNTR = 0; ATTACK_CNTR < 7; ATTACK_CNTR++)
ATTACK_LOOP
		LDA	#$00
		STA	TEMPO_CNTR

; Формирование таблицы длительностей нот для текущего темпа.
; Из таблиц DURATION_FOR_TEMPO_NORMAL, DURATION_FOR_TEMPO_DOTTED,
; DURATION_FOR_TEMPO_TRIPLET берутся базовые значения длительности, равные
; 1/512 длительности целой ноты, ноты с точкой и триольной ноты.
; Длительности последовательно умножаются на 2 и записываются в массив
; из 10 элементов так, что 0-й элемент - это целая нота, 1-й - половинная
; и т.д. 9-й элемент - базовое значение.

; for (TEMPO_CNTR = 0; TEMPO_CNTR < 16; TEMPO_CNTR++)
TEMPO_LOOP
		LDA	TEMPO_CNTR
		ASL	A
		TAY
		LDA	DURATION_FOR_TEMPO_NORMAL, Y
		STA	ACC_NORMAL_LO
		LDA	DURATION_FOR_TEMPO_DOTTED, Y
		STA	ACC_DOTTED_LO
		LDA	DURATION_FOR_TEMPO_TRIPLET, Y
		STA	ACC_TRIPLET_LO
		INY
		LDA	DURATION_FOR_TEMPO_NORMAL, Y
		STA	ACC_NORMAL_HI
		LDA	DURATION_FOR_TEMPO_DOTTED, Y
		STA	ACC_DOTTED_HI
		LDA	DURATION_FOR_TEMPO_TRIPLET, Y
		STA	ACC_TRIPLET_HI
		LDY	#$13

DUR_ARRAY_LOOP
		LDA	ACC_NORMAL_HI
		STA	DURATIONS_NORMAL, Y
		LDA	ACC_DOTTED_HI
		STA	DURATIONS_DOTTED, Y
		LDA	ACC_TRIPLET_HI
		STA	DURATIONS_TRIPLET, Y
		DEY
		LDA	ACC_NORMAL_LO
		STA	DURATIONS_NORMAL, Y
		LDA	ACC_DOTTED_LO
		STA	DURATIONS_DOTTED, Y
		LDA	ACC_TRIPLET_LO
		STA	DURATIONS_TRIPLET, Y
		CLC
		ASL	ACC_NORMAL_LO
		ROL	ACC_NORMAL_HI
		CLC
		ASL	ACC_DOTTED_LO
		ROL	ACC_DOTTED_HI
		CLC
		ASL	ACC_TRIPLET_LO
		ROL	ACC_TRIPLET_HI
		DEY
		BPL	DUR_ARRAY_LOOP
		LDA	#$00
		STA	NOTE_TYPE_CNTR

; for (NOTE_TYPE_CNTR = 0; NOTE_TYPE_CNTR < 4; NOTE_TYPE_CNTR++)
NOTE_TYPE_LOOP
		LDA	#$00
		STA	DURATION_CNTR

; for (DURATION_CNTR = 0; DURATION_CNTR < 7; DURATION_CNTR++)
NOTE_DUR_LOOP
		JSR	CALC_NOTE_TIME
		LDA	ATTACK_TIME_LO
		JSR	WRITE_TABLE
		LDA	ATTACK_TIME_HI
		JSR	WRITE_TABLE
		LDA	DECAY_TIME_LO
		JSR	WRITE_TABLE
		LDA	DECAY_TIME_HI
		JSR	WRITE_TABLE
		INC	DURATION_CNTR
		LDA	DURATION_CNTR
		CMP	#$07
		BNE	NOTE_DUR_LOOP
; добивка блока нулями до размера 32 байта
		LDA	#$00
		JSR	WRITE_TABLE
		JSR	WRITE_TABLE
		JSR	WRITE_TABLE
		JSR	WRITE_TABLE
		INC	NOTE_TYPE_CNTR
		LDA	NOTE_TYPE_CNTR
		CMP	#$04
		BNE	NOTE_TYPE_LOOP
		INC	TEMPO_CNTR
		LDA	TEMPO_CNTR
		CMP	#$10
		BEQ	NEXT_ATTACK_VAL
		JMP	TEMPO_LOOP

NEXT_ATTACK_VAL
		INC	ATTACK_CNTR
		LDA	ATTACK_CNTR
		CMP	#$07
		BEQ	INIT_PITCH_TABLES
		JMP	ATTACK_LOOP

INIT_PITCH_TABLES
		LDA	TABLES_PTRLO
		STA	ZP_A4			; ?? не используется драйвером
		LDA	TABLES_PTRHI
		STA	ZP_A5			; ?? не используется драйвером
		LDA	#$00
		STA	PITCH_TABLE_CNTR

; формирование таблиц высот
; for (PITCH_TABLE_CNTR= 0; PITCH_TABLE_CNTR < 6; PITCH_TABLE_CNTR++)
NOTE_PITCH_ARRAY_LOOP
		LDX	#$00

NOTE_PITCH_LOOP
		LDA	NOTE_PITCH, X
		JSR	WRITE_TABLE
		INX
		CPX	#$AA
		BNE	NOTE_PITCH_LOOP
		LDA	PITCH_TABLE_CNTR
		ASL	A
		TAX
		LDA	TABLES_PTRLO
		STA	ZP_D0, X		; ?? не используется драйвером
		LDA	TABLES_PTRHI
		STA	ZP_D1, X		; ?? не используется драйвером
		INC	PITCH_TABLE_CNTR
		LDA	PITCH_TABLE_CNTR
		CMP	#$06
		BNE	NOTE_PITCH_ARRAY_LOOP
		LDX	#$00

NOTE_TO_SEMITONE_LOOP
		LDA	NOTE_TO_SEMITONE, X
		JSR	WRITE_TABLE
		INX
		CPX	#$40
		BNE	NOTE_TO_SEMITONE_LOOP
		LDA	TABLES_PTRLO
		STA	ZP_F0			; ?? не используется драйвером
		LDA	TABLES_PTRHI
		STA	ACC_NORMAL_LO
		LDX	#$00

OCTAVE_TO_SEMITONE_LOOP
		LDA	OCTAVE_TO_SEMITONE, X
		JSR	WRITE_TABLE
		INX
		CPX	#$07
		BNE	OCTAVE_TO_SEMITONE_LOOP
		LDA	TABLES_PTRLO
		STA	ZP_DC			; ?? не используется драйвером
		LDA	TABLES_PTRHI
		STA	ZP_DD			; ?? не используется драйвером
		LDY	#$00
		LDA	#>TABLES_START
		STA	DUR_TABLES_PTR

; инициализация массива указателей на таблицы длительностей
DUR_TABLES_INDEX_LOOP
		LDA	#$00
		STA	(TABLES_PTRLO), Y	; ?? лишняя операция
		JSR	WRITE_TABLE
		LDA	DUR_TABLES_PTR
		STA	(TABLES_PTRLO), Y	; ?? лишняя операция
		JSR	WRITE_TABLE
		LDA	#$80
		STA	(TABLES_PTRLO), Y	; ?? лишняя операция
		JSR	WRITE_TABLE
		LDA	DUR_TABLES_PTR
		STA	(TABLES_PTRLO), Y	; ?? лишняя операция
		JSR	WRITE_TABLE
		INC	DUR_TABLES_PTR
		LDA	DUR_TABLES_PTR
		CMP	#$78
		BMI	DUR_TABLES_INDEX_LOOP
		RTS


;*******************************************************************************
; Получение двух частей длительности ноты: времени атаки и времени затухания.
; Соотношение времен атаки и затухания задается переменной ATTACK_CNTR:
;	0 соответствует времени атаки 1/8 и времени затухания 7/8
;	1 соответствует времени атаки 2/8 и времени затухания 6/8 и т.д.
; Длительность возвращается в тактах таймера. 
; На входе:
;	ATTACK_CNTR - длительность атаки (0 - 6)
;	DURATION_CNTR - длительность ноты (0 - целая, 6 - 1/64)
; На выходе:
;	ATTACK_TIME_LO, ATTACK_TIME_HI - длительность атаки в тактах таймера
;	DECAY_TIME_LO, DECAY_TIME_HI - длительность затухания в тактах таймера
;*******************************************************************************
CALC_NOTE_TIME
		LDA	ATTACK_CNTR
		BEQ	CALC_ATTACK_0
		CMP	#$01
		BEQ	CALC_ATTACK_1
		CMP	#$02
		BEQ	CALC_ATTACK_2
		CMP	#$03
		BNE	CALC_IS_ATTACK_4
		JMP	CALC_ATTACK_3

CALC_IS_ATTACK_4
		CMP	#$04
		BNE	CALC_IS_ATTACK_5
		JMP	CALC_ATTACK_4

CALC_IS_ATTACK_5
		CMP	#$05
		BNE	CALC_IS_ATTACK_6
		JMP	CALC_ATTACK_5

CALC_IS_ATTACK_6
		JMP	CALC_ATTACK_6

CALC_ATTACK_0
		LDA	DURATION_CNTR	; attack = 1/8, decay 7/8
		STA	DUR_INDEX
		INC	DUR_INDEX
		INC	DUR_INDEX
		INC	DUR_INDEX
		LDA	DUR_INDEX

CALC_ATTACK_TIME
		JSR	GET_BASE_DURATION
		LDA	BASE_DUR_LO
		STA	ATTACK_TIME_LO
		LDA	BASE_DUR_HI
		STA	ATTACK_TIME_HI

CALC_DECAY_TIME
		LDA	DURATION_CNTR
		JSR	GET_BASE_DURATION
		LDA	BASE_DUR_LO
		SEC
		SBC	ATTACK_TIME_LO
		STA	DECAY_TIME_LO
		LDA	BASE_DUR_HI
		SBC	ATTACK_TIME_HI
		STA	DECAY_TIME_HI
		RTS

CALC_ATTACK_1
		LDA	DURATION_CNTR	; attack = 2/8, decay 6/8
		STA	DUR_INDEX
		INC	DUR_INDEX
		INC	DUR_INDEX
		LDA	DUR_INDEX
		JMP	CALC_ATTACK_TIME

CALC_ATTACK_2
		LDA	DURATION_CNTR	; attack = 3/8, decay 5/8
		STA	DUR_INDEX
		INC	DUR_INDEX
		INC	DUR_INDEX
		INC	DUR_INDEX
		LDA	DUR_INDEX
		JSR	GET_BASE_DURATION
		LDA	BASE_DUR_LO
		STA	TEMP_DUR_LO
		LDA	BASE_DUR_HI
		STA	TEMP_DUR_HI
		DEC	DUR_INDEX
		LDA	DUR_INDEX
		JSR	GET_BASE_DURATION
		CLC
		LDA	BASE_DUR_LO
		ADC	TEMP_DUR_LO
		STA	ATTACK_TIME_LO
		LDA	BASE_DUR_HI
		ADC	TEMP_DUR_HI
		STA	ATTACK_TIME_HI
		JMP	CALC_DECAY_TIME

CALC_ATTACK_3
		LDA	DURATION_CNTR	; attack = 4/8, decay 4/8
		STA	DUR_INDEX
		INC	DUR_INDEX
		LDA	DUR_INDEX
		JSR	GET_BASE_DURATION
		LDA	BASE_DUR_LO
		STA	ATTACK_TIME_LO
		STA	DECAY_TIME_LO
		LDA	BASE_DUR_HI
		STA	ATTACK_TIME_HI
		STA	DECAY_TIME_HI
		RTS

CALC_ATTACK_4
		JSR	CALC_ATTACK_2

EXCHANGE_ATTACK_DECAY
		LDA	ATTACK_TIME_HI
		STA	BASE_DUR_HI
		LDA	ATTACK_TIME_LO
		STA	BASE_DUR_LO
		LDA	DECAY_TIME_HI
		STA	ATTACK_TIME_HI
		LDA	DECAY_TIME_LO
		STA	ATTACK_TIME_LO
		LDA	BASE_DUR_LO
		STA	DECAY_TIME_LO
		LDA	BASE_DUR_HI
		STA	DECAY_TIME_HI
		RTS

CALC_ATTACK_5
		JSR	CALC_ATTACK_1
		JMP	EXCHANGE_ATTACK_DECAY

CALC_ATTACK_6
		JSR	CALC_ATTACK_0
		JMP	EXCHANGE_ATTACK_DECAY


;*******************************************************************************
; 	Запись значения аккумулятора в таблицу
;*******************************************************************************
WRITE_TABLE
		STY	SAVE_Y
		LDY	#$00
		STA	(TABLES_PTRLO), Y
		INC	TABLES_PTRLO
		BNE	WRITE_TABLE2
		INC	TABLES_PTRHI

WRITE_TABLE2
		LDY	SAVE_Y
		RTS

; Таблицы базовой длительности (равной 1/512 целой ноты)
; для 16 темпов в тактах таймера (1/7980 с)
; NORMAL - обычные ноты,
; DOTTED - ноты с точкой (длительность 3/2 от обычной),
; TRIPLET - триольные ноты (длительность 1/3 от обычной)

DURATION_FOR_TEMPO_NORMAL
		.word	108, 102,  96,  90,  84,  78,  72,  66
		.word	 60,  54,  48,  42,  36,  30,  24,  18

DURATION_FOR_TEMPO_DOTTED
		.word	162, 153, 144, 135, 126, 117, 108,  99
		.word	 90,  81,  72,  63,  54,  45,  36,  27

DURATION_FOR_TEMPO_TRIPLET
		.word	 36,  34,  32,  30,  28,  26,  24,  22
		.word	 20,  18,  16,  14,  12,  10,   8,   6

DURATIONS_NORMAL
		.fill	20

DURATIONS_DOTTED
		.fill	20

DURATIONS_TRIPLET
		.fill	20

DUR_TABLES_PTR
		.byte	$00

DURATION_CNTR
		.byte	$00

NOTE_TYPE_CNTR
		.byte	$00

TEMPO_CNTR
		.byte	$00

ATTACK_CNTR
		.byte	$00

PITCH_TABLE_CNTR
		.byte	$00

ATTACK_TIME_LO
		.byte	$00

ATTACK_TIME_HI
		.byte	$00

DECAY_TIME_LO
		.byte	$00

DECAY_TIME_HI
		.byte	$00


;*******************************************************************************
; Получение длительности ноты в тактах таймера для указанного типа и обозначения
; длительности
; На входе:
;	A - длительность ноты (0 - целая, 9 - 1/512)
;	NOTE_TYPE_CNTR - тип
; На выходе:
;	BASE_DUR_LO, BASE_DUR_HI - длительность в тактах
;*******************************************************************************
GET_BASE_DURATION
		ASL	A
		TAY
		LDA	NOTE_TYPE_CNTR
		BEQ	GET_NORMAL_DUR
		CMP	#$01
		BEQ	GET_TRIPLET_DUR
		CMP	#$02
		BEQ	GET_DOTTED_DUR
		JMP	GET_DOTTED_TRIPLET_DUR

GET_TRIPLET_DUR
		LDA	DURATIONS_TRIPLET, Y
		STA	BASE_DUR_LO
		LDA	DURATIONS_TRIPLET + 1, Y
		STA	BASE_DUR_HI
		RTS

GET_NORMAL_DUR
		LDA	DURATIONS_NORMAL, Y
		STA	BASE_DUR_LO
		LDA	DURATIONS_NORMAL + 1, Y
		STA	BASE_DUR_HI
		RTS

; Если нота одновременно триольная и с точкой, то ее длительность равна
; (1/3) * (3/2) = 1/2 обычной ноты. Поэтому значение берется из массива
; длительностей обычных нот, но со сдвигом на 1 позицию
GET_DOTTED_TRIPLET_DUR
		LDA	DURATIONS_NORMAL + 2, Y
		STA	BASE_DUR_LO
		LDA	DURATIONS_NORMAL + 3, Y
		STA	BASE_DUR_HI
		RTS

GET_DOTTED_DUR
		LDA	DURATIONS_DOTTED, Y
		STA	BASE_DUR_LO
		LDA	DURATIONS_DOTTED + 1, Y
		STA	BASE_DUR_HI
		RTS


; Таблица делителей, задающих высоту нот
NOTE_PITCH
		.word	$FEFB, $F0A2, $E317, $D64F
		.word	$CA40, $BEDE, $B420, $A9FD
		.word	$A06C, $9765, $8EDF, $86D5
		.word	$7F3E, $7815, $7153, $6AF2
		.word	$64EE, $5F40, $59E3, $54D4
		.word	$500E, $4B8D, $474C, $4349
		.word	$3F80, $3BED, $388E, $355F
		.word	$325E, $2F88, $2CDB, $2A55
		.word	$27F3, $25B4, $2395, $2194
		.word	$1FB0, $1DE8, $1C39, $1AA2
		.word	$1922, $17B8, $1663, $1520
		.word	$13F0, $12D1, $11C1, $10C2
		.word	$0FD0, $0EEC, $0E15, $0D4A
		.word	$0C8B, $0BD6, $0B2C, $0A8B
		.word	$09F3, $0964, $08DC, $085D
		.word	$07E4, $0773, $0707, $06A2
		.word	$0642, $05E8, $0593, $0543
		.word	$04F7, $04AF, $046C, $042C
		.word	$03F0, $03B7, $0382, $034F
		.word	$0320, $02F3, $02C8, $02A0
		.word	$027A, $0257, $0235, $0215
		.word	$01F7

; Таблица перекодировки номера ноты в номер полутона
NOTE_TO_SEMITONE
		.byte	$00, $00, $01, $03, $05, $06, $08, $0A
		.byte	$0C, $0D, $0F, $11, $12, $14, $16, $18
		.byte	$00, $01, $02, $04, $06, $07, $09, $0B
		.byte	$0D, $0E, $10, $12, $13, $15, $17, $19
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $00, $00, $00, $00, $00, $00, $00
		.byte	$00, $FF, $01, $02, $04, $05, $07, $09
		.byte	$0B, $0C, $0E, $10, $11, $13, $15, $17

; Таблица перекодировки номера октавы в номер полутона
OCTAVE_TO_SEMITONE
		.byte	0, 12, 24, 36, 48, 60, 72

; Это неинициализированная память
		.byte	$00, $AF, $DC, $DD, $DB, $D4, $DD, $DB
		.byte	$AE, $DD, $DB, $A9, $DD, $8D, $8D, $A8
		.byte	$C1, $AD, $C8, $AC, $D0, $A9, $DB, $B1
		.byte	$AD, $B7, $DD, $DB, $D4, $DD, $DB, $AE
		.byte	$DD, $DB, $A9, $DD, $AD, $F5, $E4, $C1
		.byte	$D0, $C8, $F9, $C5, $8D, $8D, $CC, $A8
		.byte	$B1, $AD, $B7, $A9, $DB, $D4, $DD, $DB
		.byte	$AE, $DD, $DB, $A9, $DD, $A0, $AD, $E4
		.byte	$EC, $E9, $D4, $C5, $EC, $F8, $C8, $CF
		.byte	$C3, $D4, $F8, $8D, $8D, $D3, $A8, $B1
		.byte	$AD, $B1, $B6, $A9, $A0, $AD, $A0, $D4
		.byte	$C5, $CD, $F0, $8D, $8D, $D4, $A8, $CF
		.byte	$E7, $BA, $B1, $AD, $B3, $A9, $A8, $E6
		.byte	$BA, $B0, $AD, $B7, $A9, $DB, $BC, $BE
		.byte	$DD, $AD, $D4, $C5, $CD, $E2, $D0, $8D
		.byte	$8D, $A8, $BC, $BE, $A9, $A0, $AD, $A0
		.byte	$E9, $FA, $CD, $C5, $C8, $C5, $C8, $E9
		.byte	$C5, $A0, $E7, $D0, $CF, $CD, $CB, $CF
		.byte	$C3, $D4, $E9, $8D, $8D, $DA, $A8, $B1
		.byte	$AD, $B7, $A9, $A0, $AD, $A0, $C1, $D4
		.byte	$C1, $CB, $C1, $AD, $FA, $C1, $D4, $F5
		.byte	$D8, $C1, $C8, $E9, $C5, $8D, $8D, $D2
		.byte	$A8, $B0, $AD, $B1, $B5, $A9, $A0, $AD
		.byte	$A0, $D0, $C1, $C3, $C3, $D4, $D0, $CF
		.byte	$EA, $CB, $C1, $8D, $8D, $CF, $A8, $B0
		.byte	$AD, $B6, $A9, $A0, $AD, $A0, $FA, $C1
		.byte	$E4, $C1, $C8, $E9, $C5, $A0, $CF, $CB
		.byte	$D4, $C1, $C2, $F9, $8D

		.end
