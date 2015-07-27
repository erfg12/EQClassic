Harakiri Sept. 23 2009

You will need a unix or cygwin envirenment to rebuild the perl mapper files (perl_XXX.cpp).
The system needs the perl interpreter with xsubpp.

When you want to use a new method from i.e. Client.cpp add the method signature to client.h.
 
When you copy this dir, make sure convert utility has executable rights

chmod +x convert

Then simply call

./convert client.h Client

if you added a method to the client, it will create perl_client.cpp

Note, if you add functions with complex datatypes (i.e. Spell) you need to add a typemap for this, you first need to create
a perl mapping class for the object too.