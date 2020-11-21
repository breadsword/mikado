#include "mikado.h"

using mikado::mikado;

int main(int argc, char **argv)
{
    // create socket to connect to mqtt broker

    // construct a connection object
    int myconn;

    // construct a mikado
    auto mi = mikado<int>{myconn};

    // connect the mikado
//    mi.connect("example-client-id");

    // mi.subscribe("/testtopic/a");

    // mi.publish("/testpublish", "value");
    // mi.loop();
    // if (mi.has_received())
    {
        // output received message and topic

    }

    // mi.disconnect();

    // check for mikado's status and potential errors


    return 0;
}
