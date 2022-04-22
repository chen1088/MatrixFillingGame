package framework.combinatorics;

import javafx.util.Pair;
import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.function.Predicate;

public class Polynomial {
   // Multivariate polynomial with fixed dimension
   public LinkedHashMap<Multivariable, Integer> coeffDic = new LinkedHashMap<>();
   public int dim;
   public Polynomial(int d)
   {
      dim = d;
   }
   public Polynomial multiply(Polynomial o)
   {
      Polynomial res = new Polynomial(dim);
      for(Multivariable mv :coeffDic.keySet())
      {
         for(Multivariable mvo: o.coeffDic.keySet())
         {
            Multivariable tmp = mv.product(mvo);
            Integer tmpdelta = coeffDic.get(mv) * o.coeffDic.get(mvo);
            Integer tmpi = res.coeffDic.getOrDefault(tmp, 0) + tmpdelta;
            res.coeffDic.put(tmp,tmpi);
         }
      }
      return res;
   }

   public Polynomial add(Polynomial o)
   {
      Polynomial res = new Polynomial(dim);
      for(Multivariable t:o.coeffDic.keySet())
      {
         Integer tmp = coeffDic.getOrDefault(t, 0)+o.coeffDic.get(t);
         res.coeffDic.put(t, tmp);
      }
      return res;
   }
   public void set(Multivariable mv, Integer coeff)
   {
      coeffDic.put(mv,coeff);
   }
   public static void main(String[] args){

      Multivariable m11 = new Multivariable(new ArrayList<Integer>(Arrays.asList(1,1,0)));
      Multivariable m12 = new Multivariable(new ArrayList<Integer>(Arrays.asList(-1,-1,0)));
      Multivariable m21 = new Multivariable(new ArrayList<Integer>(Arrays.asList(1,0,1)));
      Multivariable m22 = new Multivariable(new ArrayList<Integer>(Arrays.asList(-1,0,-1)));
      Multivariable c = new Multivariable(new ArrayList<Integer>(Arrays.asList(0,0,0)));
      Multivariable m2 = new Multivariable(new ArrayList<Integer>(Arrays.asList(1,0,1)));
      Multivariable m3 = new Multivariable(new ArrayList<Integer>(Arrays.asList(-1,0,-1)));
      Polynomial p1 = new Polynomial(3);
      Polynomial p2 = new Polynomial(3);
      p1.set(c,2);
      p1.set(m11,1);
      p1.set(m12,1);
      p2.set(c,2);
      p2.set(m21,1);
      p2.set(m22,1);
      Polynomial p3 = p1.multiply(p2);
      System.out.println(p3);
//      System.out.println(m.product(m2));
//      System.out.println(m2.product(m3).isIdentity);

   }

   @Override
   public String toString(){
      return coeffDic.toString();
   }
}
class Multivariable implements Comparable<Multivariable>
{
   public ArrayList<Integer> varlist;
   public boolean isIdentity;
   public Multivariable(ArrayList<Integer> l)
   {
      varlist = l;
      isIdentity = l.stream().noneMatch(new Predicate<Integer>() {
         @Override
         public boolean test(Integer integer) {
            return integer != 0;
         }
      });
   }

   public Multivariable product(Multivariable o)
   {
      int varsize = Math.max(varlist.size(),o.varlist.size());
      ArrayList<Integer> resl = new ArrayList<>();
      for(int i = 0;i< varsize;++i) {
         Integer a = i<varlist.size()?varlist.get(i):0;
         Integer b = i<o.varlist.size()?o.varlist.get(i):0;
         resl.add(a+b);
      }
      return new Multivariable(resl);
   }
   @Override
   public int compareTo(@NotNull Multivariable o) {
      // return the first difference

      return 0;
   }
   @Override
   public boolean equals(Object o){
      // we should assume the dimension of the multivariable is equal for efficiency
      if(!(o instanceof Multivariable)) return false;
      Multivariable mv = (Multivariable) o;
      if(mv.isIdentity && isIdentity) return true;
      else if(mv.isIdentity != isIdentity) return false;
      else if(mv.varlist.size()!=varlist.size()) return false;
      else {
         for(int i = 0;i<varlist.size();++i)
         {
            if(mv.varlist.get(i)!=varlist.get(i)) return false;
         }
      }
      return true;
   }
   @Override
   public int hashCode(){
      return varlist.hashCode();
   }

   public static void main(String[] args){
      Multivariable m = new Multivariable(new ArrayList<Integer>(Arrays.asList(0,0,1)));
      Multivariable m2 = new Multivariable(new ArrayList<Integer>(Arrays.asList(1,0,1)));
      Multivariable m3 = new Multivariable(new ArrayList<Integer>(Arrays.asList(-1,0,-1)));
      System.out.println(m.product(m2));
      System.out.println(m2.product(m3).isIdentity);

   }
   @Override
   public String toString(){
      return varlist.toString();
   }
}
