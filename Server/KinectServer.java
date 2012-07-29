import processing.core.*;
import processing.net.*;

public class KinectServer extends PApplet {
    Server s;
    Client c;
    String input;
    int handPos[] = new int[12];
    int handDepth;
    String s_handPos[];

    public void setup() 
    {
      size(800, 600);
      background(127);
      stroke(0);
      s = new Server(this, 10002); 
    }

    public void draw() 
    {
      background(127);
      smooth();
      noStroke();
      fill(255);
      c = s.available();
      if (c != null) {
        input = c.readString();
        s_handPos = split(input, " ");
        if ( s_handPos.length == 13 ) {
            for ( int i=0; i < 12; i++ ) 
                handPos[i] = Integer.valueOf(s_handPos[i]).intValue();
            handDepth = Integer.valueOf(s_handPos[12]).intValue();
            for ( int i=0; i < 11; i++ ) 
                ellipse(handPos[i], handPos[++i], 20, 20);
        }
      }
      //TODO One connection, see c++ kinect program
      //s.disconnect(c);
    }

    //public void mousePressed() {
        //c.stop();
        //s.stop();
        //s = null;
    //}

    public void serverEvent(Server server, Client client) 
    {
        println("A new client has connected: " + client.ip());
        //if ( client.ip().equals("127.0.0.1") )
            //server.write("OK\n");
        //else
            //server.write("DENIED\n");
    }

    void disconnectEvent(Client client) 
    {
        print("Client disconnected: ");
        println(client.ip());
    }

    static public void main(String args[]) 
    {
        PApplet.main(new String[] {"KinectServer"});
    }
}
