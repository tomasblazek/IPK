# IPK-Projekt 1 Dokumentace

#### JMÉNO  
> ftrest- klient  
> ftrestd- server

#### PŘEHLED
>ftrestd	[-p PORT] [-r KOŘENOVÝ_ADRESÁŘ]  
>ftrest		PŘÍKAZ VZDÁLENÝ_ADRESAŘ [LOKÁLN_ADRESÁŘ]

#### POPIS
>Programy ftrest/ftrestd reprezentují klient/server aplikaci, která umožňuje práci se soubory a složkami. Komunikuje pomocí HTTP a využívá rozhraní RESTful API. Server (ftresd) se zapíná jako první a následně čeká na příchozí komunikace. Klient (ftrest) se zapínam s povínnými parametry PŘÍKAZ, podle které se vybere RESTful operace a VZDÁLENÁ_ADRESÁŘ. Ten by měl mít tvar: http://hostname:port/user-account/remote-path. Při nezadání portu se se volí implitní číslo portu 6677. 

#### MOŽNOSTI
>-r KOŘENOVÝ_ADRESÁŘ specifikuje kořenový adrsář, kde budou ukládány soubory pro jednotlivé uživatele, defaultní hodnota je aktuální.

>-p PORT specifikuje port, na kterém bude server naslouchat, implicitní 6677. Celkový rozsah <0,65535>.


#### PŘÍKAZ
>Na pozici PŘÍKAZ se zadává operace z množiny (del,get,put,lst,mkd,rmd):
	
>**del** Smaže soubor určený REMOTE-PATH na serveru.

>**get** zkopíruje soubor z REMOTE-PATH do aktuálního lokálního adresáře či na místo určené pomocí LOCAL-PATH je-li uvedeno.

>**put** zkopíruje soubor z LOCAL-PATH do adresáře REMOTE-PATH.

>**lst**  vypíše obsah vzdáleného adresáře na standardní výstup.

>**mkd** vytvoří adresář specifikovaný v REMOTE-PATH na serveru

>**rmd** odstraní adresář specifikovaný V REMOTE-PATH ze serveru


#### AUTOR
>Tomáš Blažek (login: xblaze31)
