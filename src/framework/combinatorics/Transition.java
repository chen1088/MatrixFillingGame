package framework.combinatorics;

public class Transition {
   public int statep;
   public int stateq;
   public int alpha;
   public Transition(int p, int q, int a)
   {
      statep = p;
      stateq = q;
      alpha = a;
   }
   public String getFrom(){
      return Integer.toString(statep);
   }
   public String getTo(){
      return Integer.toString(stateq);
   }
   public String getLetter(){
      return Integer.toString(alpha);
   }
}
