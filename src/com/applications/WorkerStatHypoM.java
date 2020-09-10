package com.applications;

import javax.swing.*;

public class WorkerStatHypoM extends MFGSwingWorker{

   public WorkerStatHypoM(BMContainer c) {
      super(c);
   }
   public WorkerStatHypoM(BMContainer c, JTextField tf, JProgressBar pb, JCheckBox cb)
   {
      super(c,tf,pb,cb);
   }

   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatHypoM(container,resultField,progressBar,enable);
   }

   @Override
   public void SetAppearance() {
      enable.setText("Hypothesis");
   }

   @Override
   protected String doInBackground() {
      boolean[][] ignm = container.GetFullIgnoreMap();
      Integer[][] m = container.GetMaximalConf();
      Integer[][] mfilled = BMUtil.GreedyFillMFG2(m,container.Height(),container.Width());
      boolean res = BMUtil.HasBlank(mfilled,container.Height(),container.Width());
      //boolean res = BMUtil.HasDiagRect_naive(mfilled,container.Height(),container.Width());
      return !res ? "Works" : "Fails";
   }
}
