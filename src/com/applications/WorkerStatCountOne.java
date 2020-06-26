package com.applications;

import javax.swing.*;

public class WorkerStatCountOne extends MFGSwingWorker{

   public WorkerStatCountOne(BMContainer c, JTextField tf, JProgressBar pb) {
      super(c, tf, pb);
   }

   @Override
   protected String doInBackground() {
      //TODO: fast count for all-one configurations
      return null;
   }
}
