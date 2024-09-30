Utils.c:
	Acest fisier contine functiile de trimitere si primire al unui packet intreg
de pe socketul respectiv, cat mai contine si functia care compara un stirng s2
si vede daca se potriveste cu topicul s1 (acesta poate include si wildcarduri)

Utils.h:
	Contine trei structuri:
->packet_udp: care contine tipicul mesajului, tipul acestuia si mesajul de transmis
->packet_tcp: care contine tipul packetului (0-exit de la client la server, 1-
subscribe, 2-unsubscribe, 255-exit de la server la client), ipul clientului care
a transmis packetul udp original, portul acestuia, si payloadul care poate fi ori
un topic, ori un packet udp.
->client: care contine ID-ul unui client, maxtopics care reprezinta dimensiunea
maxima alocata actual a vectorului de topicuri, topicsize care reprezinta cate
topicuri are un client, online care reprezinta daca clientul este online si topics
care este vectorul de topicuri la care este abonat clientul.

Subscriber.c:
	Subscriber.c este implementarea unui client tcp si functioneaza in felul ur-
mator:
->mai intai o sa ne conectam la server si o sa folosim multiplexarea I/O pentru
a putea primi mesaje atat de la server cat si de la stdin fara a fi necesara una
pentru cealalta, astfel daca primim o notificare si vedem ca e de la stdin o sa
citim din bufferul stdin-ului comanda respectiva si o sa o executam (adica o sa
trimitem la server mesajul necesar: exit, subscribe + topic sau unsubscribe + topic),
altfel daca am primit un mesaj de la server o sa il parsam si o sa i afisam la
stdout.

Server.c:
	Server.c este implementarea serverului si functioneaza in felul urmator:
->in server o sa avem un vector pentru clientii tcp(informatile acestora) si unul
pentru file-descriptori serverului folositi cu multiplexare I/O, important este
ca socketul prin care este conectat clientul tcp cu nr i se va afla pe pozitia
i in vectorul de filedescriptori, astfel putem accesa usor infromatiile clientilor
dupa ce am aflat de pe ce socket au venit datele.
->mai intai o sa deschidem doi socketi, unul folosind tco si unul folosind udp, si
o sa ascultam pe ambii socketi cat si pe stdin.
->daca primim o notificare de la stdin atunci citim comanda respectiva si o sa
o executam (singura optiune valida este exit, caz in care o sa trimitem la toti
clientii tcp semnalul de inchidere si o sa inchidem serverul)
->daca vine un packet udp atunci o sa vedem la ce clienti tcp trebuie sa trimitem
pachetul respectiv si o sa il dam mai departe acestora
->daca vine pe socketul de tcp atunci o sa creeam un nou socket pentru clientul
care vrea sa se conecteze si o sa verificam daca acesta deja exista si este conectat:
daca da, atunci ii transmitem sa se inchida si inchedm socketul, daca exista dar
este offline atunci punem socketul la pozitia respectiva a clientului in vectorul
de fds si ascultam pe el, si daca nu exista inainte atunci o sa creeam un client
nou in baza de date si o sa ne punem sa ascultam si pe socketul respectiv
->iar daca vine un mesaj de la un client tcp, atunci o sa executam comanda trimisa
de acesta (exit, subscribe sau unsubscribe)
