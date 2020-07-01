package com.applications;

public class Tuple<T1,T2> {
   private T1 item1;
   private T2 item2;
   public Tuple(T1 t1, T2 t2){
      set1(t1);
      set2(t2);
   }

   public T1 get1() {
      return item1;
   }

   public void set1(T1 item1) {
      this.item1 = item1;
   }

   public T2 get2() {
      return item2;
   }

   public void set2(T2 item2) {
      this.item2 = item2;
   }
}
