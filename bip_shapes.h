void draw_cross(int x_center,int y_center,int width) {
	draw_horizontal_line(y_center, x_center-(width/2), x_center+(width/2));//высота,начало,конец
	draw_vertical_line(x_center, y_center-(width/2), y_center+(width/2));//старт,начало,конец
	//draw_filled_rect(0, 48, 56, 81);//начало X/начало У/конец Х/конец У
}

void draw_hex(int x_center,int y_center,char diametr) {
	for (char i = 0; (diametr/2);i++) {
		draw_horizontal_line(y_center + i, x_center - ((diametr-i) / 2), x_center + ((diametr-i) / 2));//высота,начало,конец
		if (i == (diametr / 2)) {
			break;
		}
	}
	for (char m = 0; -(diametr/2);m--) {
		draw_horizontal_line(y_center + m, x_center - ((diametr+m) / 2), x_center + ((diametr+m) / 2));//высота,начало,конец
		if (m == -(diametr / 2)) {
			break;
		}
	}
}
void drawCircle(int r) 
{ 
    // Consider a rectangle of size N*N 
    int N = 2*r+1; 
  
    int x, y;  // Coordinates inside the rectangle 
  
    // Draw a square of size N*N. 
    for (int i = 0; i < N; i++) 
    { 
        for (int j = 0; j < N; j++) 
        { 
            // Start from the left most corner point 
            x = i-r; 
            y = j-r; 
  
            // If this point is inside the circle, print it 
            if (x * x + y * y <= r * r + 1)
                draw_filled_rect(x, y, x+1, y+1);//начало X/начало У/конец Х/конец У
            else // If outside the circle, print space 
                text_out(" ", x, y);
            text_out(" ", x, y); 
        } 
        text_out("\n", x, y);
    } 
} 