#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <pcap.h>
#include <speex/speex.h>

#define ETHER_TYPE_IP (0x0800)

int main(int argc, char *argv[])
{
    pcap_t *handle; 
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr header;
    const u_char *packet;

    if (argc < 2) {
        fprintf(stderr, "Usage: pplay <capture_file>\n");
        return EXIT_FAILURE;
    }
    handle = pcap_open_offline(argv[1], errbuf);
    if (!handle) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    FILE *pcm_file = fopen("output.raw", "wb");
    if (!pcm_file) {
        fprintf(stderr, "Could not open %s\n", "output.raw");
        return EXIT_FAILURE;
    }

    // decoder
    SpeexBits decoder_bits;
    int frame_samples = 0;
    speex_bits_init(&decoder_bits);
    void *decoder_state = speex_decoder_init(&speex_nb_mode);
    speex_decoder_ctl(decoder_state, SPEEX_GET_FRAME_SIZE, &frame_samples);
    short *pcm_buffer = (short*)malloc(2 * frame_samples);

    while (packet = pcap_next(handle, &header)) {
        u_char *pkt_ptr = (u_char *)packet;

        // parse the first (ethernet) header, grabbing the type field 
        int ether_type = ((int)(pkt_ptr[12]) << 8) | (int)pkt_ptr[13];
        int ether_offset = 0;
        if (ether_type != ETHER_TYPE_IP)
            continue;
        pkt_ptr += 14;

        // parse the IP header 
        struct ip *ip_hdr = (struct ip *)pkt_ptr;
        if (ip_hdr->ip_p != IPPROTO_UDP)
            continue;
        pkt_ptr += sizeof(*ip_hdr);

        // parse the UDP header
        struct udphdr *udp_hdr = (struct udphdr *)pkt_ptr;
        const int udp_length = ntohs(udp_hdr->uh_ulen);
        if (ntohs(udp_hdr->uh_sport) != 40000 ||
            ntohs(udp_hdr->uh_dport) != 18806)
            continue;
        pkt_ptr += sizeof(*udp_hdr);

        // parse the RTP header
        pkt_ptr += 12;
        const int rtp_length = udp_length - (pkt_ptr - (u_char*)udp_hdr);

        // decode SPEEX
        speex_bits_read_from(&decoder_bits, pkt_ptr, rtp_length);
        speex_decode_int(decoder_state, &decoder_bits, pcm_buffer);
        fwrite(pcm_buffer, 2 * frame_samples, 1, pcm_file);
    }
    pcap_close(handle);
    fclose(pcm_file);
    return EXIT_SUCCESS;
}
