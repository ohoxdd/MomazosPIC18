	 ______           __                                            
	/\  _  \         /\ \__                                         
	\ \ \L\ \  __  __\ \ ,_\   ___                                  
	 \ \  __ \/\ \/\ \\ \ \/  / __`\                                
	  \ \ \/\ \ \ \_\ \\ \ \_/\ \L\ \                               
	   \ \_\ \_\ \____/ \ \__\ \____/                               
		\/_/\/_/\/___/   \/__/\/___/                                
																	
																	
	 ______             ___  ___                __                  
	/\__  _\          /'___\/\_ \              /\ \                 
	\/_/\ \/     ___ /\ \__/\//\ \      __     \_\ \    ___   _ __  
	   \ \ \   /' _ `\ \ ,__\ \ \ \   /'__`\   /'_` \  / __`\/\`'__\
		\_\ \__/\ \/\ \ \ \_/  \_\ \_/\ \L\.\_/\ \L\ \/\ \L\ \ \ \/ 
		/\_____\ \_\ \_\ \_\   /\____\ \__/.\_\ \___,_\ \____/\ \_\ 
		\/_____/\/_/\/_/\/_/   \/____/\/__/\/_/\/__,_ /\/___/  \/_/ 
																	
-------------------------------------------------------------------------

	Lab 7 Interfícies de Computadors Q1 2024-2025
	Projecte Inflador Pneumàtics

	Autors:
		
		Ángel Andrés Quiñones Rusanov
		Héctor Fernández de Sevilla Martínez

-------------------------------------------------------------------------

	CONTROLES:
	
	RC0 / d : Botón Incrementar presión deseada 
	RC1 / a : Botón Decrementar presión deseada 
	RC2 / s : Botón inicio inflado
	RC3 / w : Botón parada de emergencia
	
-------------------------------------------------------------------------

	INTERFAZ DE USUARIO:

	De izquierda a derecha, en la parte superior de la GLCD se muestra
	la siguiente información:

		- Indicador de estado: 
			· Tres puntos -------------> READY
			· Flechas direccionales ---> RUNNING
			· Barras vericales --------> STOPPED

		- Temperatura ambiente y presión leidas.
		- Tiempo de inflado restante
		- Barra de progreso
	
	En la mitad inferior izquierda de la GLCD se mostrarán avisos:

		- Animación 'DONE!' al terminar un inflado satisfactoriamente.
		- Advertencia de pinchazos

	En la mitad inferior derecha de la GLCD se muestra la presión
	seleccionada junto a botones que proporcionaran una respuesta 
	visual a los inputs del usuario.

	Tres LEDS conectados a los pines RA<0:2> del PIC18 que dan
	información acerca del estado actual de la apliación.

		
-------------------------------------------------------------------------

	Estructura del código:

	Diferentes partes del programa estan en diferntes archivos .h
	dependiendo de su funcion:
		
		splash.h    Funciones del splash inicial
		utils.h     Diferentes funciones de utilidad
		typedefs.h  Definiciones de tipos de datos
		initPIC.h   Configuracion inicial del PIC18F45K22
		ADC.h       Lógica relacionada con el Analog/Digital Converter
		states.h    Acciones al cambiar el estado de la apliación
		UI.h        Dibujado de los elementos de la UI
		usart.h     Lógica y control de errores por línea série
		sprites.h   Arrays que contienen los diferentes sprites
		CCP.h       Lógica del Pulse Width Modulator
		sensors.h   Interpretar valores de los sensores de PSI/ºC
	
	Junto con el comprimido del proyecto se adjunta un .pdf y un .drawio
	con el esquema de estados de la aplicación.
		
























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
		

