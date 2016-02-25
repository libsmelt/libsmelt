#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <unistd.h>
#include <assert.h>
#include <memory> // unique_ptr

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

void smlt_tree_parse_wrapper(char* json_string,
                             uint16_t** model,
                             uint32_t** leafs,
                             uint32_t* last_node)
{

}

static void smlt_tree_config_request(const char *hostname, 
                                     const char* msg,
                                     unsigned ncores,
                                     uint16_t** model,
                                     uint32_t** leafs,
                                     uint32_t* last_node)
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
    if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;

    int socketfd ; // The socket descripter
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                  host_info_list->ai_protocol);
    if (socketfd == -1)  std::cout << "socket error " ;


    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1)  std::cout << "connect error" ;


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
            
        } else if (bytes_recieved == -1) {
        }
    }
        
    } while(bytes_recieved>0);


    assert(model != NULL);
    // Parse json
    Json::Value root;
    // Json::CharReaderBuilder rbuilder;
    // rbuilder["collectComments"] = false;
    std::string errs;
    // bool ok = Json::parseFromStream(rbuilder, rec, &root, &errs);

    Json::CharReaderBuilder rbuilder;
    Json::CharReader* reader = rbuilder.newCharReader();

    const char* input = rec.c_str();
    reader->parse(input, input+strlen(input)-1, &root, &errs);

    // Extract last node
    Json::Value ln = root.get("last_node", "");
    int lnval = ln.asInt();
    *last_node = (uint32_t)lnval;

    // Extract leaf node
    Json::Value leafs_j = root.get("leaf_nodes", "");
    *leafs = (uint32_t*) malloc(sizeof(uint32_t)*leafs_j.size());
    assert(*leafs != NULL);
    for (unsigned i=0; i<leafs_j.size(); i++) {
        *leafs[i] = leafs_j.get(i, Json::Value()).asInt();
    }
    
    Json::Value elem = root.get("model", "");
    *model = (uint16_t*)malloc(sizeof(uint16_t)*elem.size());
    assert(*model != NULL);
    int x = 0;
    int y = 0;
    for (Json::ValueIterator i=elem.begin(); i != elem.end(); i++) {
        y = 0;
        Json::Value inner = (*i);
        for (Json::ValueIterator k=inner.begin(); k != inner.end(); k++) {

            Json::Value val = (*k);
            *model[x*ncores+y] = (uint32_t) val.asInt();
            y++;
        }
        x++;
    }
    
    freeaddrinfo(host_info_list);
    shutdown(socketfd, SHUT_RDWR);
}

#define NAMELEN 1000U

void smlt_tree_generate_wrapper(unsigned ncores, 
                                uint32_t* cores,
                                char* tree_name,
                                uint16_t** model,
                                uint32_t** leafs,
                                uint32_t* last_node)
{
    // Ask the Simulator to build a model
    const char *host = get_env_str("SMLT_HOSTNAME", "");

    // Determine hostname of this machine
    char thishost[NAMELEN];
    gethostname(thishost, NAMELEN);

    if (strlen(host)>0) {
        printf("Hostname is %s \n", host);
    } else {
        printf("No hostname give, not contacting generator \n");
        return;
    }
    
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentatin"] = "\t";
    
    Json::Value root;   // 'root' will contain the root value after parsing.
    root["machine"] = thishost;
    if (tree_name == NULL) {
        root["topology"] = "adaptivetree";
    } else {
        root["topology"] = tree_name;
    }

    Json::Value coreids;
    for (unsigned i=0; i<ncores; i++)
    coreids.append(cores[i]);
    
    root["cores"] = coreids;

    std::string doc = Json::writeString(wbuilder, root);
 
    smlt_tree_config_request(host, doc.c_str(), ncores,
                             model, leafs, last_node);

}

