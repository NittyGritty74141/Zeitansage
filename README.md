# Zeitansage
Die telefonische Zeitansage der Deutschen Bundespost stark verkleinert
The telephone time announcement of the Deutsche Bundespost greatly reduced

Long time ago, this was the most precise clock. You had to dial '119' and got into the loop. There is an old video on YT
https://www.youtube.com/watch?v=BAaR1WlnShM
where you get explanation about how this huge machine is working.

Inspired by the Timespeak Scetch I found the complete announcements in one Video
https://www.youtube.com/watch?v=iJkdPHfVqCI

I extracted it to MP3 and uses Audacity to split off the huge file into useful audio snippets.

Finally, I modified the code (not yet perfect, but working) and created another "Clock" - compared to the rack in the video
it is now just a loudspeaker, a DF-Player and ESP8266 (I used NodeMCU Board, but Wemos and others will also work.

Just to mention the 1k-Resistor to be put into Rx and Tx lines - I have no clue, why, but without, the DF-Player does behave weired.

According to the above video (with three pickups) - I placed the audio files into 3 folders on the SD-Card. Just see the code, you will understand.
