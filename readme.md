#### JMÉNO  
> ftrest- klient  
> ftrestd- server

#### PŘEHLED
>ftrestd	[-p PORT] [-r KOŘENOVÁ-CESTA]  
>ftrest		PŘÍKAZ VZDÁLENÁ-CESTA [LOKÁLNÍ-CESTA]

#### POPIS
>Programy ftrest/ftrestd reprezentují klient/server aplikaci, která umožňuje práci se soubory a složkami. Komunikuje pomocí HTTP a využívá rozhraní RESTful API. Server (ftresd) se zapíná jako první a následně čeká na příchozí komunikace. Klient (ftrest) se zapínam s povinnými parametry PŘÍKAZ, podle které se vybere RESTful operace a VZDÁLENÁ-CESTA. Ten by měl mít tvar: http://hostname:port/user-account/remote-path. Při nezadání portu se se volí implitní číslo portu 6677. 

#### POPIS IMPLEMENTACE
>Nejprve zkontroluji argumenty příkazové řádky a následně z nich postupně získám hostname,port,user-account,remote-path u klienta. U serveru získám port a kořenový adresař. Z prvního argumentu dostanu požadovanou operaci. Podle typu operace odešlu zprávu serveru s požadavkem ve tvaru RESTful operace a hlavičky, popřídě daty pokud se jedná o operaci put. Server si přečte hlavičku požadavku a délku zprávy. Požadavek zpracuje a odešle odpověď ve formatu RESTful odpovědi, hlavičky a dat pokud jsou nějaké požadovány. Pokud se vyskytne chyba, odešle se na místo dat chybová zpráva. Následně je klient ukončen a server vyčkává na další požadavek.

#### MOŽNOSTI
>-r KOŘENOVÁ-CESTA specifikuje kořenový adresář, kde budou ukládány soubory pro jednotlivé uživatele, defaultní hodnota je aktuální.

>-p PORT specifikuje port, na kterém bude server naslouchat, implicitní 6677. Celkový rozsah <0,65535>.


#### PŘÍKAZ
>Na pozici PŘÍKAZ se zadává operace z množiny (del,get,put,lst,mkd,rmd):
	
>**del** Smaže soubor určený parametrem VZDÁLENÁ-CESTA na serveru.

>**get** Zkopíruje soubor z VZDÁLENÁ-CESTA do aktuálního lokálního adresáře či na místo určené pomocí LOKÁLNÍ-CESTA je-li uvedeno.

>**put** Zkopíruje soubor z LOKÁLNÍ-CESTA do VZDÁLENÁ-CESTA na serveru.

>**lst** Vypíše obsah vzdáleného adresáře na standardní výstup.

>**mkd** Vytvoří adresář specifikovaný VZDÁLENÁ-CESTA na serveru.

>**rmd** Odstraní adresář specifikovaný VZDÁLENÁ-CESTA ze serveru.


#### AUTOR
>Tomáš Blažek (login: xblaze31)
