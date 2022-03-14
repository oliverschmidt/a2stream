# A2Stream
A2Stream simultaneously receives an ***.a2stream*** file from any HTTP server and plays it through the stock Apple II speaker in a consistent 46 cycle loop resulting in a stable 22050Hz sample rate.

Hardware requirements:
* Enhanced //e or IIgs
* [Uthernet II](http://a2retrosystems.com/products.htm) in any slot

To create an ***.a2stream*** file:
* Create a header-less ***.raw*** file with 22050Hz mono 32-bit-float PCM data (e.g. with [Audacity](https://www.audacityteam.org/))
* Generate an ***.a2stream*** file from the ***.raw*** file with ***gena2stream.exe***

To stream an ***.a2stream*** file:
* Put the ***.a2stream*** file onto any HTTP server
* Run ***A2STREAM.SYSTEM*** and point it to the URL of the ***.a2stream*** file

To config the Ethernet slot:
* Put the Uthernet II in slot 3 <ins>or</ins>
* Create a file named ***ETHERNET.SLOT***. Only the first byte of that file is relevant. This byte can either represent your Uthernet II slot as binary value (e.g. $04 for slot 4), as ASCII digit (e.g. $34 for slot 4) or as Apple TEXT digit (e.g. $B4 for slot 4).

To get decent sound quality on the enhanced //e:
* Create a file named ***OUTPUT.TAPE***. The content of that file is not relevant.
* Connect an amplifier/speaker to the tape output jack

To get decent sound quality on the IIgs:
* Connect an amplifier/speaker to the headphone jack
