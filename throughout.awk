BEGIN {
	init=0;
	cnt = 0;
	FS="[() \t]";#field seperator is ')' or'('or ' '
	myScrIP = "10.1.1.6";#This is the link that we pay attention to
	myDstIP = "10.1.1.1";
}
{
	action = $1;
	time = $2;
	namespace=$3;
	#printf("%d\n",NF);
	for (i=1;i<=NF;i++)#find packet ID
	{
		if ($i ~ /id/) #if $i field matches "id"
           myPacketID = $(i+1);#record the id of the packet for future use
		else if ($i ~ /length:/) #if $i field matches "length:"
           myLength =  $(i+1);#record the length of the packet for future use
		else if ($i ~ />/) #if $i field matches ">"
		{
            srcIP = $(i-1);
            dstIP = $(i+1);
            if(match(srcIP, myScrIP) && match(dstIP, myDstIP) )#link matches
            {
				packet_id = myPacketID;
                pktsize = myLength;
                #record send time of the packet
                if (start_time[packet_id]==0)
                {
					start_time[packet_id]=time;
                }
                if (action=="r")
                {
                     if(end_time_packet[packet_id] ==0 )#the first time we receive this packet
                    {
                        end_time_packet[packet_id] = time;#record time according to id
                        packetCNT[packet_id]=cnt;
                        pkt_byte_sum[cnt+1]=pkt_byte_sum[cnt]+ pktsize;
                        end_time[cnt]=time;
                        cnt++;
                    }#if(end_time_packet[packet_id] ==0)
					else#not the 1st time we receive this packet,so we update receive time
                    {
                    #printf("*****duplicate packetID: %s,cnt=%s,end_time_old=%s,end_time new: %s\n",packet_id,cnt,end_time[packetCNT[packet_id]], time);
                      end_time[packetCNT[packet_id]]=time;
					}
                }#if (action=="r")
            }#if match(srcIP, myScrIP)

        }#else if ($i ~ />/) #if $i field matches ">"
	}#for (i=1;i<=NF;i++)#find packet ID
}

END {
        printf("%s\t%s\n", end_time[0], 0);
        for(j=1 ; j<cnt;j++){
            throughput = (pkt_byte_sum[j] / (end_time[j] - start_time[0]))*8/1000;
            printf("%s\t%s\n", end_time[j], throughput );
        }
}
