/*
	(С) Волков Максим 2019 ( Maxim.N.Volkov@ya.ru )
	(С) ugers 2020
	Календарь v1.0  
	Приложение простого календаря.
	Алгоритм вычисления дня недели работает для любой даты григорианского календаря позднее 1583 года. 
	Григорианский календарь начал действовать в 1582 — после 4 октября сразу настало 15 октября. 
	Календарь от 1600 до 3000 года
	Функции перелистывания каленаря вверх-вниз - месяц, стрелками год
	При нажатии на название месяца устанавливается текущая дата
	v.1.1
	- исправлены переходы в при запуске из быстрого меню
	График работы v.2.0
	Приложение "График работы" показывает в какие дни вам на работу, когда вы работаете по графику.
	На выбор графики: 1/1,1/2,1/3,2/2,д.н.о.в.,2д2в2н2в,2д2н2в,3д3в3н3в а так же выключить график(сделать приложение обычным календарем)
	Т.к. все работают по разному то в настройках(свайп вправо) есть смещение дней, чтобы каждый мог настроить график под себя.
	В приложении строиться график до конца текущего года. При наступлении следующего года нужно менять смещение дня.
*/
#include "graph_work.h"
//#define DEBUG_LOG
//#define BipEmulator
#ifdef BipEmulator
	#include "libbip.h"
#else
	#include <libbip.h>
#endif

//	структура меню экрана календаря
struct regmenu_ menu_calend_screen = {
						55,
						1,
						0,
						dispatch_calend_screen,
						key_press_calend_screen, 
						calend_screen_job,
						0,
						show_calend_screen,
						0,
						0
					};
int main(int param0, char** argv){	//	переменная argv не определена
	show_calend_screen((void*) param0);
}
void show_calend_screen (void *param0){
	struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
	struct calend_ *	calend;								//	указатель на данные экрана
	#ifdef DEBUG_LOG
		log_printf(5, "[show_calend_screen] param0=%X; *temp_buf_2=%X; menu_overlay=%d", (int)param0, (int*)get_ptr_temp_buf_2(), get_var_menu_overlay());
		log_printf(5, " #calend_p=%X; *calend_p=%X", (int)calend_p, (int)*calend_p);
	#endif
if ( (param0 == *calend_p) && get_var_menu_overlay()){ // возврат из оверлейного экрана (входящий звонок, уведомление, будильник, цель и т.д.)
	#ifdef DEBUG_LOG
		log_printf(5, "  #from overlay");
		log_printf(5, "\r\n");
	#endif	
	calend = *calend_p;						//	указатель на данные необходимо сохранить для исключения 
											//	высвобождения памяти функцией reg_menu
	#ifdef BipEmulator
		*calend_p = (calend_*)NULL;						//	обнуляем указатель для передачи в функцию reg_menu
	#else
		*calend_p = NULL;						//	обнуляем указатель для передачи в функцию reg_menu
	#endif
	// 	создаем новый экран, при этом указатели temp_buf_1 и temp_buf_2 были равны 0 и память не была высвобождена	
	reg_menu(&menu_calend_screen, 0); 		// 	menu_overlay=0
	*calend_p = calend;						//	восстанавливаем указатель на данные после функции reg_menu
	draw_month(0, calend->month, calend->year);
}else{ 			// если запуск функции произошел из меню, 
	#ifdef DEBUG_LOG
		log_printf(5, "  #from menu");
		log_printf(5, "\r\n");
	#endif
	// создаем экран
	reg_menu(&menu_calend_screen, 0);
	// выделяем необходимую память и размещаем в ней данные
	*calend_p = (struct calend_ *)pvPortMalloc(sizeof(struct calend_));
	calend = *calend_p;		//	указатель на данные
	// очистим память под данные
	_memclr(calend, sizeof(struct calend_));
	#ifdef BipEmulator
		calend->proc = (Elf_proc_*)param0;
	#else
		calend->proc = param0;
	#endif
	// запомним адрес указателя на функцию в которую необходимо вернуться после завершения данного экрана
	if ( param0 && calend->proc->elf_finish ) 			//	если указатель на возврат передан, то возвоащаемся на него
		calend->ret_f = calend->proc->elf_finish;
	else					//	если нет, то на циферблат
		calend->ret_f = show_watchface;
	struct datetime_ datetime;
	_memclr(&datetime, sizeof(struct datetime_));
		// получим текущую дату
	get_current_date_time(&datetime);
	calend->day 	= datetime.day;
	calend->month 	= datetime.month;
	calend->year 	= datetime.year;
	// считаем опции из flash памяти, если значение в флэш-памяти некорректное то берем первую схему
	// текущая цветовая схема хранится о смещению 0
	// очистим память под данные
	_memclr(&datetime, sizeof(struct datetime_));
	struct calend_opt_ 	calend_opt;							//	опции календаря
	_memclr(&calend_opt, sizeof(struct calend_opt_));
	ElfReadSettings(ELF_INDEX_SELF, &calend_opt, OPT_OFFSET_CALEND_OPT, sizeof(struct calend_opt_));
	if (calend_opt.color_scheme < COLOR_SCHEME_COUNT){
		calend->color_scheme = calend_opt.color_scheme;
	}else{ 
		calend->color_scheme = 0;
	}
	if((0 < calend_opt.vibration_opt) && (calend_opt.vibration_opt < 2)){
		calend->vibration_opt = calend_opt.vibration_opt;
	}else{
		calend->vibration_opt = 0;
	}
	if((0 < calend_opt.yearoffset_opt) && (calend_opt.yearoffset_opt < 21)){
		calend->yearoffset = calend_opt.yearoffset_opt;
	}else{
		calend->yearoffset = 0;
	}
	if((0 < calend_opt.graphik_opt) && (calend_opt.graphik_opt < 9)){
		calend->graphik = calend_opt.graphik_opt;
	}else{
		calend->graphik = 0;
	}
	if((30000 < calend_opt.inactivity_period_opt) && (calend_opt.inactivity_period_opt < 260000)){
		calend->inactivity_period = calend_opt.inactivity_period_opt;
	}else{
		calend->inactivity_period = 30000;
	}
	// очистим память под данные
	_memclr(&calend_opt, sizeof(struct calend_opt_));
//Напоминание о смене года и необходимости поменять смещение	
	if((calend->day == 31)&&(calend->month == 12)){
		set_bg_color(COLOR_BLACK);
		fill_screen_bg();
		set_graph_callback_to_ram_1();
		load_font(); // подгружаем шрифты
		set_fg_color(COLOR_WHITE);
		text_out_center("Завтра нужно", 88, 10);//надпись,ширина,высота
		text_out_center("поменять", 88, 35);//надпись,ширина,высота
		text_out_center("смещение дня!", 88, 60);//надпись,ширина,высота		
		text_out_center("Tomorrow you", 88, 85);//надпись,ширина,высота
		text_out_center("need change", 88, 110);//надпись,ширина,высота
		text_out_center("day offset!", 88, 135);//надпись,ширина,высота
		repaint_screen_lines(0, 176);
		vTaskDelay(1000);	
	}
//
	draw_month(calend->day, calend->month, calend->year);
}
// при бездействии погасить подсветку и не выходить
set_display_state_value(8, 1);
set_display_state_value(2, 1);
// таймер на job на 20с где выход.
set_update_period(1, calend->inactivity_period);
}
void draw_month(unsigned int day, unsigned int month, unsigned int year){
struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
struct calend_ *	calend = *calend_p;						//	указатель на данные экрана
/*
 0:	CALEND_COLOR_BG					фон календаря;
 1:	CALEND_COLOR_MONTH				цвет названия текущего месяца;
 2:	CALEND_COLOR_YEAR				цвет текущего года,	фон  чисел рабочего дня в ночь;
 3:	CALEND_COLOR_WORK_NAME			цвет названий дней будни;
 4: CALEND_COLOR_HOLY_NAME_BG		фон	 названий дней выходные, фон  чисел текущего месяца выходные при работе;
 5:	CALEND_COLOR_HOLY_NAME_FG		цвет названий дней выходные;
 6:	CALEND_COLOR_SEPAR				цвет разделителей календаря;
 7:	CALEND_COLOR_NOT_CUR_WORK		цвет чисел НЕ текущего месяца будни, фон  чисел рабочего дня в день;
 8:	CALEND_COLOR_NOT_CUR_HOLY_BG	фон  чисел НЕ текущего месяца выходные;
 9:	CALEND_COLOR_NOT_CUR_HOLY_FG	цвет чисел НЕ текущего месяца выходные;
10:	CALEND_COLOR_CUR_WORK			цвет чисел текущего месяца будни;
11:	CALEND_COLOR_CUR_HOLY_BG		фон  чисел текущего месяца выходные;
12:	CALEND_COLOR_CUR_HOLY_FG		цвет чисел текущего месяца выходные;
13: CALEND_COLOR_TODAY_BG			фон  чисел текущего дня; 		bit 31 - заливка: =0 заливка цветом фона, =1 только рамка, фон как у числа не текущего месяца 
14: CALEND_COLOR_TODAY_FG			цвет чисел текущего дня;
*/
static unsigned char short_color_scheme[COLOR_SCHEME_COUNT][15] = 	
/* черная тема без выделения выходных*/	{//		0				1				2				3				4				5				6
										{COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE,
										//		7				8				9			10					11				12				13
										COLOR_SH_GREEN, COLOR_SH_BLACK, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_YELLOW,
										//		14
										COLOR_SH_BLACK},
										//		0				1			2					3			4				5				6
/* белая тема без выделения выходных*/	{COLOR_SH_WHITE, COLOR_SH_BLACK, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_BLACK,
										//		7				8			9				10				11				12			13
										COLOR_SH_YELLOW, COLOR_SH_WHITE, COLOR_SH_AQUA, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_BLUE,
										//		14
										COLOR_SH_WHITE},
										//		0				1			2					3			4				5				6
/* черная тема с выделением выходных*/	{COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE,
										//		7				8			9				10				11				12 				13
										COLOR_SH_GREEN, COLOR_SH_RED, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_AQUA,
										//		14
										COLOR_SH_BLACK},
										//		0				1			2					3			4				5				6
/* белая тема с выделением выходных*/	{COLOR_SH_WHITE, COLOR_SH_BLACK, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_BLACK,
										//		7				8			9				10				11			12				13
										COLOR_SH_YELLOW, COLOR_SH_RED, COLOR_SH_BLUE, COLOR_SH_BLACK, COLOR_SH_RED, COLOR_SH_BLACK, COLOR_SH_BLUE,
										//		14
										COLOR_SH_WHITE},
										//		0				1			2					3			4				5				6
/* черная тема без выделения выходных*/	{COLOR_SH_BLACK, COLOR_SH_YELLOW, COLOR_SH_AQUA, COLOR_SH_WHITE, COLOR_SH_RED, COLOR_SH_WHITE, COLOR_SH_WHITE,
/*с рамкой выделения сегодняшнего дня*/	//		7				8			9					10				11				12				13
										COLOR_SH_GREEN, COLOR_SH_BLACK, COLOR_SH_AQUA, COLOR_SH_YELLOW, COLOR_SH_BLACK, COLOR_SH_WHITE, COLOR_SH_AQUA|(1<<7),
										//		14
										COLOR_SH_BLACK},
										};
int color_scheme[COLOR_SCHEME_COUNT][15];
for (unsigned char i=0;i<COLOR_SCHEME_COUNT;i++)
	for (unsigned char j=0;j<15;j++){
	color_scheme[i][j]  = (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_RED)  ?COLOR_RED   :0;	//	составляющая красного цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_GREEN)?COLOR_GREEN :0;	//	составляющая зеленого цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)COLOR_SH_MASK)&COLOR_SH_BLUE) ?COLOR_BLUE  :0;	//	составляющая синего цвета
	color_scheme[i][j] |= (((unsigned int)short_color_scheme[i][j]&(unsigned char)(1<<7))) ?(1<<31) :0;				//	для рамки	
}
char text_buffer[24];
char *weekday_string_ru[] = {"??", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
char *weekday_string_en[] = {"??", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
char *weekday_string_it[] = {"??", "Lu", "Ma", "Me", "Gi", "Ve", "Sa", "Do"};
char *weekday_string_fr[] = {"??", "Lu", "Ma", "Me", "Je", "Ve", "Sa", "Di"};
char *weekday_string_es[] = {"??", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá", "Do"};
char *weekday_string_short_ru[] = {"?", "П", "В", "С", "Ч", "П", "С", "В"};
char *weekday_string_short_en[] = {"?", "M", "T", "W", "T", "F", "S", "S"};
char *weekday_string_short_it[] = {"?", "L", "M", "M", "G", "V", "S", "D"};
char *weekday_string_short_fr[] = {"?", "M", "T", "W", "T", "F", "S", "S"};
char *weekday_string_short_es[] = {"?", "L", "M", "X", "J", "V", "S", "D"};
char *monthname_ru[] = {
	     "???",		
	     "Январь", 		"Февраль", 	"Март", 	"Апрель",
		 "Май", 		"Июнь",		"Июль", 	"Август", 
		 "Сентябрь", 	"Октябрь", 	"Ноябрь",	"Декабрь"};
char *monthname_en[] = {
	     "???",		
	     "January", 	"February", 	"March", 		"April", 	
		 "May", 		"June",			"July", 		"August",
		 "September", 	"October", 		"November",		"December"};
char *monthname_it[] = {
	     "???",		
	     "Gennaio", 	"Febbraio", "Marzo", 	"Aprile", 
		 "Maggio", 		"Giugno",   "Luglio", 	"Agosto", 
		 "Settembre", 	"Ottobre", 	"Novembre", "Dicembre"};
char *monthname_fr[] = {
	     "???",		
	     "Janvier",		"Février",	"Mars",		"Avril", 
		 "Mai", 		"Juin", 	"Juillet",	"Août", 
		 "Septembre", 	"Octobre", 	"Novembre", "Décembre"};
char *monthname_es[] = {
	     "???",		
	     "Enero", 		"Febrero", 	"Marzo", 		"Abril", 	
		 "Mayo", 		"Junio", 	"Julio",		"Agosto", 	
		 "Septiembre", 	"Octubre", 	"Noviembre", 	"Diciembre"};
unsigned char day_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
char**	weekday_string;
char**	weekday_string_short;
char**	monthname;
switch (get_selected_locale()){
		case locale_ru_RU:{
			weekday_string = weekday_string_ru;
			weekday_string_short = weekday_string_short_ru;
			monthname = monthname_ru;
			break;
		}
		case locale_it_IT:{
			weekday_string = weekday_string_it;
			weekday_string_short = weekday_string_short_it;
			monthname = monthname_it;
			break;
		}
		case locale_fr_FR:{
			weekday_string = weekday_string_fr;
			weekday_string_short = weekday_string_short_fr;
			monthname = monthname_fr;
			break;
		}
		case locale_es_ES:{
			weekday_string = weekday_string_es;
			weekday_string_short = weekday_string_short_es;
			monthname = monthname_es;
			break;
		}
		default:{
			weekday_string = weekday_string_en;
			weekday_string_short = weekday_string_short_en;
			monthname = monthname_en;
			break;
		}
	}
_memclr(&text_buffer, 24);
set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_BG]);	//	фон календаря
fill_screen_bg();
set_graph_callback_to_ram_1();
load_font(); // подгружаем шрифты
_sprintf(&text_buffer[0], " %d", year);
int month_text_width = text_width(monthname[month]);
int year_text_width  = text_width(&text_buffer[0]);
set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_MONTH]);		//	цвет месяца
text_out(monthname[month], (176-month_text_width-year_text_width)/2 ,5); 	// 	вывод названия месяца
set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_YEAR]);		//	цвет года
text_out(&text_buffer[0],  (176+month_text_width-year_text_width)/2 ,5); 	// 	вывод года
text_out("←", 5		 ,5); 		// вывод стрелки влево
text_out("→", 176-5-text_width("→"),5); 		// вывод стрелки вправо
int calend_name_height = get_text_height();
set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_SEPAR]);
draw_horizontal_line(CALEND_Y_BASE, H_MARGIN, 176-H_MARGIN);	// Верхний разделитель названий дней недели
draw_horizontal_line(CALEND_Y_BASE+1+V_MARGIN+calend_name_height+1+V_MARGIN, H_MARGIN, 176-H_MARGIN);	// Нижний  разделитель названий дней недели
//draw_horizontal_line(175, H_MARGIN, 176-H_MARGIN);	// Нижний  разделитель месяца
// Названия дней недели
for (unsigned i=1; (i<=7);i++){
	if ( i>5 ){		//	выходные
		set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG]);
		set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_FG]);
	} else {		//	рабочие
		set_bg_color(color_scheme[calend->color_scheme][CALEND_COLOR_BG]);	
		set_fg_color(color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
	}
	// отрисовка фона названий выходных    
	int pos_x1 = H_MARGIN +(i-1)*(WIDTH  + H_SPACE);
	int pos_y1 = CALEND_Y_BASE+V_MARGIN+1;
	int pos_x2 = pos_x1 + WIDTH;
	int pos_y2 = pos_y1 + calend_name_height;
	// фон для каждого названия дня недели
	draw_filled_rect_bg(pos_x1, pos_y1, pos_x2, pos_y2);
	// вывод названий дней недели. если ширина названия больше чем ширина поля, выводим короткие названия
	if (text_width(weekday_string[1]) <= (WIDTH - 2))
		text_out_center(weekday_string[i], pos_x1 + WIDTH/2, pos_y1 + (calend_name_height-get_text_height())/2 );	
	else 
		text_out_center(weekday_string_short[i], pos_x1 + WIDTH/2, pos_y1 + (calend_name_height-get_text_height())/2 );	
}
int calend_days_y_base = CALEND_Y_BASE+1+V_MARGIN+calend_name_height+V_MARGIN+1;
	char LeapYear;
if (isLeapYear(year)>0){
	day_month[2]=29;
	LeapYear = 1;
}else{
	LeapYear = 0;
}
unsigned char d=wday(1,month, year);
unsigned char m=month;
	struct datetime_ datetime;
	_memclr(&datetime, sizeof(struct datetime_));
	get_current_date_time(&datetime);	// получим текущую дату
	int current_year = datetime.year;
	_memclr(&datetime, sizeof(struct datetime_));
if (d>1) {
     m=(month==1)?12:month-1;
     d=day_month[m]-d+2;
	}
// числа месяца
for (unsigned char i=1; (i<=7*6);i++){
	 unsigned char row = (i-1)/7;
     unsigned char col = (i-1)%7+1;
    _sprintf (&text_buffer[0], "%2.0d", d);
	int bg_color = 0;
	int fg_color = 0;
	int frame_color = 0; 	// цветрамки
	int frame = 0; 		// 1-рамка; 0 - заливка
	// если текущий день текущего месяца
	if ( (m==month)&&(d==day) ){
		if ( color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] & (1<<31) ) {// если заливка отключена  только рамка
			// цвет рамки устанавливаем CALEND_COLOR_TODAY_BG, фон внутри рамки и цвет текста такой же как был
			frame_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG &COLOR_MASK]);
			// рисуем рамку
			frame = 1;
			if ( col > 5 ){ // если выходные 
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_BG]); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_FG]);
			} else {		//	если будни
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
			};
		} else { 						// если включена заливка	
			if ( col > 5 ){ // если выходные 
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] & COLOR_MASK); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_FG]);
			} else {		//	если будни
				bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_BG] &COLOR_MASK); 
				fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_TODAY_FG]);
			};
		};		
	// если рабочие дни текущего месяца
	} else if (calend->graphik != 8 ){
		if ((calend->year == year)  && (d >= 1)){
			for (char nm = 1; nm <= 12; nm++) {
				char mouthoffset;
				char mo_num;
				if (calend->graphik == 0) { // 1/1
					if (m == nm) {
					if (LeapYear > 0) { //проверка высокостный год или нет
						if (m == 1) mo_num = 1;			//январь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 2) mo_num = 0;	//февраль 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 3) mo_num = 1;	//март 1/1		//смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 4) mo_num = 0;	//апрель 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 5) mo_num = 0;	//май 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 6) mo_num = 1;	//июнь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 7) mo_num = 1;	//июль 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 8) mo_num = 0;	//август 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 9) mo_num = 1;	//сентябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 10) mo_num = 1;	//октябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 11) mo_num = 0;	//ноябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 12) mo_num = 0;	//декабрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
					}else {
						if (m == 1) mo_num = 1;			//январь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 2) mo_num = 0;	//февраль 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 3) mo_num = 0;	//март 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 4) mo_num = 1;	//апрель 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 5) mo_num = 1;	//май 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 6) mo_num = 0;	//июнь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 7) mo_num = 0;	//июль 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 8) mo_num = 1;	//август 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 9) mo_num = 0;	//сентябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 10) mo_num = 0;	//октябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 11) mo_num = 1;	//ноябрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
						else if (m == 12) mo_num = 1;	//декабрь 1/1	  //смещение дня       1 = 1,3 / 0 = 2,4
					}
					mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 2 == 0) { //проверка на нечетность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 1) { // 1/2
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num = 2;			//январь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 2) mo_num = 0;	//февраль 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 3) mo_num = 2;	//март 1/2		//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 4) mo_num = 0;	//апрель 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 5) mo_num = 0;	//май 1/2		//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 6) mo_num = 1;	//июнь 1/2		//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 7) mo_num = 1;	//июль 1/2		//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 8) mo_num = 2;	//август 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 9) mo_num = 0;	//сентябрь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 10) mo_num = 0;	//октябрь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 11) mo_num = 1;	//ноябрь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 12) mo_num = 1;	//декабрь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
						}else {
							if (m == 1) mo_num = 2;			//январь 1/2	//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 2) mo_num = 0; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 3) mo_num = 1; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 4) mo_num = 2; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 5) mo_num = 2; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 6) mo_num = 0; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 7) mo_num = 0; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 8) mo_num = 1; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 9) mo_num = 2; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 10) mo_num = 2; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 11) mo_num = 0; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
							else if (m == 12) mo_num = 0; 					//смещение дня       2 = 1,4 / 0 = 3,6 / 1 = 2,5
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 3 == 0) { //проверка на нечетность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 2) { // 1/3
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 3) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 4) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 5) mo_num=2; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 6) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 7) mo_num=3; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 8) mo_num=2; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 9) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 10) mo_num=3; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 11) mo_num=2; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 12) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
						}else {
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 3) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 4) mo_num=3; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 5) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 6) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 7) mo_num=2; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 8) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 9) mo_num=0; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 10) mo_num=2; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 11) mo_num=1; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
							else if (m == 12) mo_num=3; 					//смещение дня       1 = 1,5 / 0 = 2,6 / 3 = 3,7 / 2 = 4,8
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 2 == 0) { //проверка на нечетность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if (((d + mouthoffset) / 2) % 2 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 3) { // 2/2
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 3) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 4) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 5) mo_num=2; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 6) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 7) mo_num=3; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 8) mo_num=2; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 9) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 10) mo_num=3; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 11) mo_num=2; 					//смещение дня       1 = 1,3 / 0 = 2,4 
							else if (m == 12) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4 
						}else {
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 3) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 4) mo_num=3; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 5) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 6) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 7) mo_num=2; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 8) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 9) mo_num=0; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 10) mo_num=2; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 11) mo_num=1; 					//смещение дня       1 = 1,3 / 0 = 2,4
							else if (m == 12) mo_num=3; 					//смещение дня       1 = 1,3 / 0 = 2,4
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 2 == 0) { //проверка на четность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if (((d + mouthoffset) / 2) % 2 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
				}
				else if (calend->graphik == 4) { // д.н.о.в
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 3) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 4) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 5) mo_num=2; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 6) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 7) mo_num=3; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 8) mo_num=2; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 9) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 10) mo_num=3; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 11) mo_num=2; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 12) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
						}else {
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 2) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 3) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 4) mo_num=3; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 5) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 6) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 7) mo_num=2; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 8) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 9) mo_num=0; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 10) mo_num=2; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 11) mo_num=1; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
							else if (m == 12) mo_num=3; 					//смещение дня       1 = 1,2 / 0 и 4 = 2,3 / 3 = 3,4 / 2 = 1
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 2 == 0) { //проверка на нечетность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if (((d + mouthoffset) / 2) % 2 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 5) { // 2д2н2в
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 2) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 3) mo_num=1; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 4) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 5) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 6) mo_num=3; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 7) mo_num=3; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 8) mo_num=4; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 9) mo_num=5; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 10) mo_num=5; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 11) mo_num=0; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 12) mo_num=0; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
						}else {
							if (m == 1) mo_num=1; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 2) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 3) mo_num=0; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 4) mo_num=1; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 5) mo_num=1; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 6) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 7) mo_num=2; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 8) mo_num=3; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 9) mo_num=4; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 10) mo_num=4; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 11) mo_num=5; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
							else if (m == 12) mo_num=5; 					//смещение дня       1 = 1,2 день / 0 = 1 вых 2,3 день / 2 = 1 день 2,3 ночь / 3 = 1,2 ночь
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset + 4) % 6 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if ((d + mouthoffset + 3) % 6 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					//ночные
					if ((d + mouthoffset + 1) % 6 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if ((d + mouthoffset + 2) % 6 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 6) { // 2д2в2н2в
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=1; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 2) mo_num=0; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 3) mo_num=5; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 4) mo_num=4; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 5) mo_num=2; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 6) mo_num=1; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 7) mo_num=7; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 8) mo_num=6; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 9) mo_num=5; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 10) mo_num=3; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 11) mo_num=2; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 12) mo_num=0; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
						}else {
							if (m == 1) mo_num=1; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 2) mo_num=0; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 3) mo_num=4; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 4) mo_num=3; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 5) mo_num=1; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 6) mo_num=0; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 7) mo_num=6; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 8) mo_num=5; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 9) mo_num=4; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 10) mo_num=2; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 11) mo_num=1; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
							else if (m == 12) mo_num=7; 					//смещение дня       0 = 1,2 ночь / 1 = 1 ночь / 2 = 1,2 выходной / 3 = 1 выходной / 4 = 1,2 день / 5 = 1 день / 6 = 1,2 выходной
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 2 == 0) { //проверка на четность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if (((d + mouthoffset) / 2) % 2 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if ((d + mouthoffset - 2) % 8 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if ((d + mouthoffset - 3) % 8 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				}
				else if (calend->graphik == 7) { // 3д3в3н3в
					if (m == nm) {
						if (LeapYear > 0) { //проверка высокостный год или нет
							if (m == 1) mo_num=6; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 2) mo_num=1; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 3) mo_num=6; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 4) mo_num=1; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 5) mo_num=7; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 6) mo_num=2; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 7) mo_num=8; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 8) mo_num=3; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 9) mo_num=10; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 10) mo_num=4; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 11) mo_num=11; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 12) mo_num=5; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
						}else {
							if (m == 1) mo_num=6; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 2) mo_num=1; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 3) mo_num=5; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 4) mo_num=0; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 5) mo_num=6; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 6) mo_num=1; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 7) mo_num=7; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 8) mo_num=2; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 9) mo_num=9; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 10) mo_num=3; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 11) mo_num=10; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
							else if (m == 12) mo_num=4; 					//смещение дня       0 = 1,2,3 - день / 1 = 2,3 день / 2 = 1 день / 3 = 1,2,3 вых / 4 = 1,2 вых/ 5 = 1 вых/ 6 = 1,2,3 ночь
						}
						mouthoffset = mo_num + calend->yearoffset;
					}
					if ((d + mouthoffset) % 3 == 0) { //проверка на четность
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if (((d + mouthoffset) / 3) % 2 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_HOLY_NAME_BG] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					}
					else {		//добавляем нечетные и получаем график
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_WORK_NAME]);
					};
					if ((d + mouthoffset + 3) % 12 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if ((d + mouthoffset + 2) % 12 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
					if ((d + mouthoffset + 1) % 12 == 0) { //из четных выбираем четные чтобы было смещение
						bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_YEAR] & COLOR_MASK);
						fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
					};
				};
			}
		}else{
			if (col > 5){  // если выходные 
				if (month == m){
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_BG]); 
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_FG]);
				} else {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_BG]); 
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_FG]);
				}
			} else {		//	если будни
				if (month == m){
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
				} else {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]); 
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
				}
			}
		}
	} else {
		if (calend->graphik == 8) {
			if (col > 5) {  // если выходные 
				if (month == m) {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_BG]);
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_HOLY_FG]);
				}
				else {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_BG]);
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_HOLY_FG]);
				}
			}
			else {		//	если будни
				if (month == m) {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]);
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_CUR_WORK]);
				}
				else {
					bg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_BG]);
					fg_color = (color_scheme[calend->color_scheme][CALEND_COLOR_NOT_CUR_WORK]);
				}
			}
		}
	}
	//  строка: от 7 до 169 = 162рх в ширину 7 чисел по 24рх на число 7+(22+2)*6+22+3
	//  строки: от 57 до 174 = 117рх в высоту 6 строк по 19рх на строку 1+(17+2)*5+18
	// отрисовка фона числа
	int pos_x1 = H_MARGIN +(col-1)*(WIDTH  + H_SPACE);
	int pos_y1 = calend_days_y_base + V_MARGIN + row *(HEIGHT + V_SPACE)+1;
	int pos_x2 = pos_x1 + WIDTH;
	int pos_y2 = pos_y1 + HEIGHT;	
	if (frame){
		// выводим число
		set_bg_color(bg_color);
		set_fg_color(fg_color);
		text_out_center(&text_buffer[0], pos_x1+WIDTH/2, pos_y1+(HEIGHT-get_text_height())/2);
		//	рисуем рамку
		set_fg_color(frame_color);
		draw_rect(pos_x1, pos_y1, pos_x2-1, pos_y2-1);	
	} else {
		// рисуем заливку
		set_bg_color(bg_color);
		draw_filled_rect_bg(pos_x1, pos_y1, pos_x2, pos_y2);
		// выводим число
		set_fg_color(fg_color);
		text_out_center(&text_buffer[0], pos_x1+WIDTH/2, pos_y1+(HEIGHT-get_text_height())/2);
	};
	if ( d < day_month[m] ) {
		d++;
	} else {
		d=1; 
		m=(m==12)?1:(m+1);
	}
}
};
unsigned char wday(unsigned int day,unsigned int month,unsigned int year)
{
    signed int a = (14 - month) / 12;
    signed int y = year - a;
    signed int m = month + 12 * a - 2;
    return 1+(((day + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12) % 7) +6)%7;
}
unsigned char isLeapYear(unsigned int year){
    unsigned char result = 0;
    if ( (year % 4)   == 0 ) result++;
    if ( (year % 100) == 0 ) result--;
    if ( (year % 400) == 0 ) result++;
return result;
}
void key_press_calend_screen(){
	struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	// при достижении таймера обновления выходим
	#ifdef BipEmulator
		show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_RIGHT);	
	#else
		show_menu(calend->ret_f, (unsigned int)show_calend_screen);
	#endif
};
void draw_calend_option_menu(int bgColor, int fgColor){
	struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	if (option != 0) {
		char* settings_string_ru[] = {
				 "Настройки", "Смещение дней", "Вибрация", 	"Вкл.",
				 "Выкл.", "График", "Время выхода,сек.", "д.н.о.в","2д2н2в","2д2в2н2в",
				 "3д3в3н3в" };
		char* settings_string_en[] = {
				 "Settings", "Days offset", "Vibration", "On",
				 "Off", "Timetable", "Exit time,sec.", "d.n.s.h","2d2n2h","2d2h2n2h",
				 "3d3h3n3h" };
		char* settings_string_it[] = {
				"Impostazioni", "Offset giorno", "Vibrazione", "On",
				"Off", "Schedule", "Ora di uscita, sec.", "d.n.s.h","2d2n2h","2d2h2n2h",
				 "3d3h3n3h" };
		char* settings_string_fr[] = {
				"Paramètres", "Décalage du jour", "Vibreur", "On",
				"Off", "Schedule", "Exit time, sec.", "d.n.s.h","2d2n2h","2d2h2n2h",
				 "3d3h3n3h" };
		char* settings_string_es[] = {
				"Configuración", "Compensación de día", "Vibrar", "On",
				"Off", "Programación", "Tiempo de salida,seg.", "d.n.s.h","2d2n2h","2d2h2n2h",
				 "3d3h3n3h" };
		char** settings_string;
		switch (get_selected_locale()) {
			case locale_ru_RU: {
				settings_string = settings_string_ru;
				break;
			}
			case locale_it_IT: {
				settings_string = settings_string_it;
				break;
			}
			case locale_fr_FR: {
				settings_string = settings_string_fr;
				break;
			}
			case locale_es_ES: {
				settings_string = settings_string_es;
				break;
			}
			default: {
				settings_string = settings_string_en;
				break;
			}
		};
		char text_yearoffset[3];
		char text_timerexit[5];
		set_bg_color(bgColor);
		fill_screen_bg();
		//опция 1 минус
		set_fg_color(COLOR_RED);
		draw_filled_rect(0, 48, 48, 81);//начало X/начало У/конец Х/конец У
		//опция 2 минус
		draw_filled_rect(0, 108, 48, 141);//начало X/начало У/конец Х/конец У
		//опция 1 плюс
		set_fg_color(COLOR_GREEN);
		draw_filled_rect(128, 48, 176, 81);//начало X/начало У/конец Х/конец У
		//опция 2 плюс
		draw_filled_rect(128, 108, 176, 141);//начало X/начало У/конец Х/конец У
		set_graph_callback_to_ram_1();
		set_fg_color(fgColor);
	#define OPT1_HEIGHT				56		//	высота текста и стрелок Опции 1
	#define OPT2_HEIGHT				116		//	высота текста и стрелок Опции 2
		if (option == 1) {
			//заголовок
			text_out_center(settings_string[0], 88, 5); //надпись,ширина,высота
			draw_horizontal_line(23, H_MARGIN, 176 - H_MARGIN);	// линия отделяющая заголовок от меню
			//опция 1 - Смещение дней
			text_out_center(settings_string[1], 88, 28);//название опции,X,Y
			_sprintf(text_yearoffset, "%d", calend->yearoffset);
			text_out_center(text_yearoffset, 88, OPT1_HEIGHT); //значение опции,X,Y
			//опция 2 - Вибрация
			text_out_center(settings_string[2], 88, 88);//название опции,X,Y
			if (calend->vibration_opt == 1) {
				text_out_center(settings_string[3], 88, OPT2_HEIGHT);//значение опции,X,Y
			}
			else {
				text_out_center(settings_string[4], 88, OPT2_HEIGHT);//значение опции,X,Y
			};
			text_out_center("↓", 88, 155);//надпись,X,Y
		}
		else if (option == 2) {
			text_out_center("↑", 88, 6);//надпись,X,Y
			//опция 1 - График
			text_out_center(settings_string[5], 88, 28);//надпись,X,Y
			switch (calend->graphik) {
				case 0: {
					text_out_center("1/1", 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 1: {
					text_out_center("1/2", 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 2: {
					text_out_center("1/3", 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 3: {
					text_out_center("2/2", 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 4: {
					text_out_center(settings_string[7], 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 5: {
					text_out_center(settings_string[8], 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 6: {
					text_out_center(settings_string[9], 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 7: {
					text_out_center(settings_string[10], 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
				case 8: {
					text_out_center(settings_string[4], 88, OPT1_HEIGHT);//надпись,X,Y
					break;
				}
			};
			//опция 2 - Время выхода
			text_out_center(settings_string[6], 88, 88);
			unsigned char timerexit = calend->inactivity_period / 1000;
			_sprintf(text_timerexit, "%d", timerexit);
			text_out_center(text_timerexit, 88, OPT2_HEIGHT); //надпись,X,Y
			//кнопка отмены
			set_fg_color(COLOR_RED);
			draw_filled_rect(0, 146, 88, 176); //начало X/начало У/конец Х/конец У
				// при достижении таймера обновления выходим
	#ifdef BipEmulator
			show_res_by_id(416, 35, 153);
	#else
			show_res_by_id(ICON_CANCEL_RED, 35, 153);
	#endif
			// кнопка сохранить
			set_fg_color(COLOR_GREEN);
			draw_filled_rect(88, 146, 176, 176);//начало X/начало У/конец Х/конец У
	#ifdef BipEmulator
			show_res_by_id(417, 125, 153);
	#else
			show_res_by_id(ICON_OK_GREEN, 125, 153);
	#endif
		};
		//опция 1 минус
		set_bg_color(COLOR_RED);
		set_fg_color(fgColor);
	#define Left_WIDTH				24		//	X стрелок Опции 1
	#define Right_WIDTH				153		//	X стрелок Опции 2
		text_out_center("←", Left_WIDTH, OPT1_HEIGHT); //опция 1 минус,X,Y
		text_out_center("←", Left_WIDTH, OPT2_HEIGHT); //опция 2 минус,X,Y
		set_bg_color(COLOR_GREEN);
		//set_fg_color(COLOR_WHITE);
		text_out_center("→", Right_WIDTH, OPT1_HEIGHT + 1); //опция 1 плюс,X,Y
		text_out_center("→", Right_WIDTH, OPT2_HEIGHT + 1); //опция 2 плюс,X,Y
		repaint_screen_lines(0, 176);
	}
}
void calend_screen_job(){
	struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	// при достижении таймера обновления выходим
	#ifdef BipEmulator
		show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_LEFT);	
	#else
		show_menu(calend->ret_f, (unsigned int)show_calend_screen);
	#endif
}
int dispatch_calend_screen (void *param){
	struct calend_**    calend_p = (struct calend_ **)get_ptr_temp_buf_2();    //  указатель на указатель на данные экрана
	struct calend_ *	calend = *calend_p;			//	указатель на данные экрана
	struct calend_opt_ calend_opt;					//	опции календаря
	struct datetime_ datetime;
	// получим текущую дату
	get_current_date_time(&datetime);
	unsigned int day;
#ifdef BipEmulator
	 struct gesture_ *gest = (gesture_*) param;
#else
	 struct gesture_ *gest = param;
#endif
	 int result = 0;
	 int fgColor = COLOR_BLACK;
	 int bgColor = COLOR_WHITE;
		if ((calend->color_scheme == 0) || (calend->color_scheme == 2) || (calend->color_scheme == 4)) {
			fgColor = COLOR_WHITE;
			bgColor = COLOR_BLACK;
		}
	//для корректного возврата на начальный экран календаря определяем какой месяц был выбран и нужно ли выделять текущий день
	if ((calend->year == datetime.year) && (calend->month == datetime.month)) {
		day = datetime.day; //выделяем
	}else {
		day = 0; //не выделяем
	}

	switch (gest->gesture){
		case GESTURE_CLICK: {
			// вибрация при любом нажатии на экран
			if (calend->vibration_opt==1){
				vibrate (1, 40, 0);
			}
			switch (option) {
			case 0: {
				if (gest->touch_pos_y < CALEND_Y_BASE) { // кликнули по верхней строке
					if (gest->touch_pos_x < 44) {
						if (calend->year > 1600) calend->year--;
					}
					else {
						if (gest->touch_pos_x > (176 - 44)) {
							if (calend->year < 3000) calend->year++;
						}
						else {
							calend->day = datetime.day;
							calend->month = datetime.month;
							calend->year = datetime.year;
						}
					}
					draw_month(day, calend->month, calend->year);
					repaint_screen_lines(1, 176);
				}
				else { // кликнули в календарь
					calend->color_scheme = ((calend->color_scheme + 1) % COLOR_SCHEME_COUNT);
					// сначала обновим экран
					if ((calend->year == datetime.year) && (calend->month == datetime.month)) {
						day = datetime.day;
					}else {
						day = 0;
					}
					draw_month(day, calend->month, calend->year);
					repaint_screen_lines(1, 176);
				}
				break;
			};
			case 1:
			case 2: {
					//смещение минус
					if ((gest->touch_pos_y > 36) && (gest->touch_pos_y < 92) && (gest->touch_pos_x > 0) && (gest->touch_pos_x < 76)) {
						if (calend->vibration_opt == 1) vibrate(2, 150, 70);
						if (option == 1) {
							if (calend->yearoffset > 0) {
								calend->yearoffset--;
							}
						}
						else if (option == 2) { //график минус
							if (calend->graphik > 0) {
								calend->graphik--;
							}
						}
						draw_calend_option_menu(bgColor,fgColor);
						//смещение плюс
					}else if ((gest->touch_pos_y > 36) && (gest->touch_pos_y < 92) && (gest->touch_pos_x > 100) && (gest->touch_pos_x < 176)) {
						if (calend->vibration_opt == 1) vibrate(2, 150, 70);
						if (option == 1) {
							if (calend->yearoffset < 20) {
								calend->yearoffset++;
							}
						}
						else if (option == 2) { //график плюс
							if (calend->graphik < 8) {
								calend->graphik++;
							}
						}
						draw_calend_option_menu(bgColor,fgColor);
						//вибрация вЫключить
					}else if ((gest->touch_pos_y > 94) && (gest->touch_pos_y < 146) && (gest->touch_pos_x > 0) && (gest->touch_pos_x < 76)) {
						if (calend->vibration_opt == 1) vibrate(2, 150, 70);
						if (option == 1) {
							if (calend->vibration_opt > 0) {
								calend->vibration_opt--;
							}
						}
						else if (option == 2) { //Таймаут минус
							if (calend->inactivity_period > 30000) { //30000=30 сек.
								calend->inactivity_period = calend->inactivity_period - 10000;
							}
						}
						draw_calend_option_menu(bgColor,fgColor);
						//вибрация включить
					}else if ((gest->touch_pos_y > 94) && (gest->touch_pos_y < 146) && (gest->touch_pos_x > 100) && (gest->touch_pos_x < 176)) {
						if (option == 1) {
							vibrate(2, 150, 70);
							if (calend->vibration_opt < 1) {
								calend->vibration_opt++;
							}
						}
						else if (option == 2) { //Таймаут плюс
							if (calend->vibration_opt == 1) vibrate(2, 150, 70);
							if (calend->inactivity_period < 250000) {//250000=250 сек.
								calend->inactivity_period = calend->inactivity_period + 10000;
							}
						}
						draw_calend_option_menu(bgColor,fgColor);
					}
					if (option == 2) {
						//кнопка отмены
						if ((gest->touch_pos_y > 146) && (gest->touch_pos_y < 176) && (gest->touch_pos_x > 0) && (gest->touch_pos_x < 88)) {
							if (calend->vibration_opt == 1) vibrate(1, 70, 0);
							option = 0;
							draw_month(day, calend->month, calend->year);
							repaint_screen_lines(0, 176);
						//кнопка сохранить
						}else if ((gest->touch_pos_y > 146) && (gest->touch_pos_y < 176) && (gest->touch_pos_x > 88) && (gest->touch_pos_x < 176)) {
							if (calend->vibration_opt == 1) vibrate(1, 70, 0);
							option = 0;
							// запишем настройки в флэш память
							calend_opt.color_scheme = calend->color_scheme;
							calend_opt.yearoffset_opt = calend->yearoffset;
							calend_opt.vibration_opt = calend->vibration_opt;
							calend_opt.graphik_opt = calend->graphik;
							calend_opt.inactivity_period_opt = calend->inactivity_period;
							ElfWriteSettings(ELF_INDEX_SELF, &calend_opt, OPT_OFFSET_CALEND_OPT, sizeof(struct calend_opt_));
							_memclr(&calend_opt, sizeof(struct calend_opt_));
							draw_month(day, calend->month, calend->year);
							repaint_screen_lines(0, 176);
						}
					}
			break;
			}//end case 2
		}
		// продлить таймер выхода при бездействии через calend->inactivity_period с
		set_update_period(1, calend->inactivity_period);
		break;
	};
		case GESTURE_SWIPE_RIGHT: {	//	свайп направо
			switch(option){
				case 0:{
					option = 1;
					draw_calend_option_menu(bgColor,fgColor);
					repaint_screen_lines(0, 176); 
					break;
				}
			}
			break;
		};
		case GESTURE_SWIPE_LEFT: {	// справа налево
			switch(option){
				case 0:{
					if ( get_left_side_menu_active()){
						set_update_period(0,0);
						void* show_f = get_ptr_show_menu_func();
						// запускаем dispatch_left_side_menu с параметром param в результате произойдет запуск соответствующего бокового экрана
						// при этом произойдет выгрузка данных текущего приложения и его деактивация.
						#ifdef BipEmulator
							dispatch_left_side_menu((gesture_*)param);
						#else
							dispatch_left_side_menu(param);
						#endif
						if ( get_ptr_show_menu_func() == show_f ){
							// если dispatch_left_side_menu отработал безуспешно (листать некуда) то в show_menu_func по прежнему будет 
							// содержаться наша функция show_calend_screen, тогда просто игнорируем этот жест
							// продлить таймер выхода при бездействии через calend->inactivity_period с
							set_update_period(1, calend->inactivity_period);
							return 0;
						}
						//	если dispatch_left_side_menu отработал, то завершаем наше приложение, т.к. данные экрана уже выгрузились
						// на этом этапе уже выполняется новый экран (тот куда свайпнули)
						Elf_proc_* proc = get_proc_by_addr(main);
						proc->ret_f = NULL;
						elf_finish(main);	//	выгрузить Elf из памяти
						return 0;
					} else { 			//	если запуск не из быстрого меню, обрабатываем свайпы по отдельности
						switch (gest->gesture){
							case GESTURE_SWIPE_RIGHT: {	//	свайп направо
								#ifdef BipEmulator
									return show_menu_animate(calend->ret_f, (unsigned int)show_calend_screen, ANIMATE_RIGHT);	
								#else
									return show_menu(calend->ret_f, (unsigned int)show_calend_screen);
								#endif
								break;
							}
							case GESTURE_SWIPE_LEFT: {	// справа налево
								//	действие при запуске из меню и дальнейший свайп влево
								break;
							}
						} /// switch (gest->gesture)
					}
				break;
				};
				case 1:{
					option = 0;
					draw_month(day, calend->month, calend->year);
					repaint_screen_lines(0, 176);
					break;
				}
			}
			break;			
		};	//	case GESTURE_SWIPE_LEFT:
		case GESTURE_SWIPE_UP: {	// свайп вверх
			switch(option){
				case 0:{
					if ( calend->month < 12 ) 
							calend->month++;
					else {
							calend->month = 1; //назнаечаем месяц
							calend->year++;
					}
					//выделяем текущий день!
					if ( (calend->year == datetime.year) && (calend->month == datetime.month) )
						day = datetime.day;
					else	
						day = 0; //если месяц отличен от текущего то не выделяем его
					draw_month(day, calend->month, calend->year);
					repaint_screen_lines(1, 176);
					// продлить таймер выхода при бездействии через calend->inactivity_period с
					set_update_period(1, calend->inactivity_period);
					break;
				}
				case 1:{
					option = 2;
					draw_calend_option_menu(bgColor,fgColor);
					repaint_screen_lines(0, 176);
					break;
				}
			}
			break;
		};
		case GESTURE_SWIPE_DOWN: {	// свайп вниз
			switch(option){
				case 0:{
					if ( calend->month > 1 ) 
							calend->month--;
					else {
							calend->month = 12;
							calend->year--;
					}
					//выделяем текущий день!
					if ( (calend->year == datetime.year) && (calend->month == datetime.month) )
						day = datetime.day;
					else	
						day = 0; //если месяц отличен от текущего то не выделяем его
					draw_month(day, calend->month, calend->year);			
					repaint_screen_lines(1, 176);
					// продлить таймер выхода при бездействии через calend->inactivity_period с
					set_update_period(1, calend->inactivity_period);
					break;
				}
				case 2:{
					option = 1;
					draw_calend_option_menu(bgColor,fgColor);
					repaint_screen_lines(0, 176);
					break;
				}
			}
		break;		
		};		
		default:{	// что-то пошло не так...
			break;
		};		
	}
	return result;
};