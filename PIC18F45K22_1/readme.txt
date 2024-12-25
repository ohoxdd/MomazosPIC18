Aqui tenemos que escribir las definiciones de funciones.

Por ejemplo, este es un buen sitio para escribir como calculamos la
temperatura en calculate_temp():

	 factorizamos R2/A
	
	    -1 * R2 * (1 - VCC/ VOUT) 
	  -------------------------------
	               A
	 		= R2/A * (VCC/VOUT - 1)

	 por propiedades de logaritmos
	
	 		ln (R2/A * (VCC/VOUT - 1))
	 		= ln(R2/A) + ln(VCC/VOUT - 1)

	 usamos ln(R2/A) previamente calculado


// avanzamos el timer para que OVF ocurra en 0.1s
// asignamos al buffer de TMR0H el valor de timer_starth
// sumamos timer_startl a tmr0l, (el buffer high escribe en TMR0H)  
// para tener en cuenta las instrucciones pasadas entre el OVF y la asignaci�n
		

