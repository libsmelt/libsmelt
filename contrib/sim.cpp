#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <unistd.h>
#include <assert.h>
#include <memory> // unique_ptr
#include <string.h>

#include "sim.h"
#include "jsoncpp/json/json.h"

#define PORT "25041"

/**
 * \brief Read from environment variable as string
 */
static char* get_env_str(const char *envname, const char *defaultvalue)
{
    char *env;
    env = getenv (envname);

    if (env==NULL) {

        return (char*) defaultvalue;
    }

    return env;
}

int smlt_tree_parse_wrapper(char* json_string,
                            unsigned ncores,
                            uint16_t** model,
                            uint32_t** leafs,
                            uint32_t* t_root,
                            uint32_t* len_model)
{
    Json::Value root;
    // Json::CharReaderBuilder rbuilder;
    // rbuilder["collectComments"] = false;
    std::string errs;
    // bool ok = Json::parseFromStream(rbuilder, rec, &root, &errs);

    Json::CharReaderBuilder rbuilder;
    Json::CharReader* reader = rbuilder.newCharReader();

    reader->parse(json_string, json_string+strlen(json_string)-1,
                  &root, &errs);
    // Extract last node
    Json::Value ln = root.get("root", "");
    int lnval = ln.asInt();
    *t_root = (uint32_t)lnval;

    // Extract leaf node

    Json::Value leafs_j = root.get("leaf_nodes", "");
    *leafs = (uint32_t*) malloc(sizeof(uint32_t)*leafs_j.size());
    assert(*leafs != NULL);

    for (unsigned i=0; i<leafs_j.size(); i++) {
        (*leafs)[i] = leafs_j.get(i, Json::Value()).asInt();
    }

    Json::Value elem = root.get("model", "");
    *model = (uint16_t*)  malloc(sizeof(uint16_t)*elem.size()*elem.size());
    assert(*model != NULL);
    int x = 0;
    int y = 0;
    *len_model = elem.size();
    for (Json::ValueIterator i=elem.begin(); i != elem.end(); i++) {
        y = 0;
        Json::Value inner = (*i);
        for (Json::ValueIterator k=inner.begin(); k != inner.end(); k++) {

            Json::Value val = (*k);
            (*model)[x*elem.size()+y] = (uint32_t) val.asInt();
            y++;
        }
        x++;
    }

    return 0;
}

static int smlt_tree_config_request(const char *hostname,
                                    const char* msg,
                                    unsigned ncores,
                                    uint16_t** model,
                                    uint32_t** leafs,
                                    uint32_t* t_root,
                                    uint32_t* len_model)
{

    int status;
    struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
    struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

    // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
    // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
    // is created in c++, it will be given a block of memory. This memory is not nessesary
    // empty. Therefor we use the memset function to make sure all fields are NULL.
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

    // Now fill up the linked list of host_info structs with google's address information.
    status = getaddrinfo(hostname, PORT, &host_info, &host_info_list);
    // getaddrinfo returns 0 on succes, or some other value when an error occured.
    // (translated into human readable text by the gai_gai_strerror function).
    if (status != 0) {
        std::cout << "getaddrinfo error" << gai_strerror(status) ;
        return 1;
    }

    int socketfd ; // The socket descripter
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                  host_info_list->ai_protocol);
    if (socketfd == -1) {
        std::cout << "socket error " ;
        return 1;
    }


    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        std::cout << "connect error" ;
        return 1;
    }


    int len = strlen(msg);
    int bytes_sent;

    bytes_sent = send(socketfd, msg, len, 0);
    assert (bytes_sent==len);

    ssize_t bytes_recieved;
    char incomming_data_buffer[1000];

    std::string rec;

    do {
        bytes_recieved = recv(socketfd, incomming_data_buffer,1000, 0);

        // If no data arrives, the program will just wait here until some data arrives.
        if (bytes_recieved > 0) {
            incomming_data_buffer[bytes_recieved] = '\0' ;

            rec.append(incomming_data_buffer);
        }
        else {
            if (bytes_recieved == 0) {
               printf("host shut down \n");
            } else if (bytes_recieved == -1) {
               printf("receive error \n");
               return 1;
            }
        }

    } while(bytes_recieved>0);


    freeaddrinfo(host_info_list);
    shutdown(socketfd, SHUT_RDWR);
    assert(model != NULL);
    char* inp = (char*) rec.c_str();

    return smlt_tree_parse_wrapper(inp, ncores, model, leafs, t_root, len_model);
}

#define NAMELEN 1000U

int smlt_tree_generate_wrapper(unsigned ncores,
                               uint32_t* cores,
                               char* tree_name,
                               uint16_t** model,
                               uint32_t** leafs,
                               uint32_t* t_root,
                               uint32_t* len_model)
{
    // Ask the Simulator to build a model
    const char *host = get_env_str("SMLT_HOSTNAME", "");
    const char *machine = get_env_str("SMLT_MACHINE", "unknown");

    // Determine hostname of this machine
    char thishost[NAMELEN];
    gethostname(thishost, NAMELEN);

    if (strlen(host)>0) {
        printf("Hostname is %s \n", host);
    } else {
        printf("No hostname give, not contacting generator \n");
        return 1;
    }

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentatin"] = "\t";

    // Override hostname if given as environment variable
    if (strcmp(machine, "unknown") != 0) {
        printf("Overriding hosname %s --> %s\n", thishost, machine);
        strncpy(thishost, machine, NAMELEN);
    }
    
    Json::Value root;   // 'root' will contain the root value after parsing.
    root["machine"] = thishost;

    const char *topo_name;

    if (tree_name == NULL) {
        printf("Getting topo from env string\n");
        topo_name = get_env_str("SMLT_TOPO", "adaptivetree");
    } else {
        topo_name = tree_name;
    }

    root["topology"] = topo_name;
    printf("Requesting topology: %s\n", topo_name);

    Json::Value coreids;

    if (cores != NULL) {
        for (unsigned i=0; i<ncores; i++)
            coreids.append(cores[i]);
    } else {
        for (unsigned i=0; i<ncores; i++)
            coreids.append(i);
    }

    root["cores"] = coreids;
    
    std::string doc = Json::writeString(wbuilder, root);

    return smlt_tree_config_request(host, doc.c_str(), ncores,
                                    model, leafs, t_root, len_model);
}
