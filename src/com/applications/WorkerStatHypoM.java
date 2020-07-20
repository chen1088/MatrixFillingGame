package com.applications;

import javax.swing.*;

public class WorkerStatHypoM extends MFGSwingWorker{

   public WorkerStatHypoM(BMContainer c, JTextField tf, JProgressBar pb) {
      super(c, tf, pb);
   }

   @Override
   protected String doInBackground() {
      boolean[][] ignm = container.GetFullIgnoreMap();
      Integer[][] m = container.GetMaximalConf();
      Integer[][] mfilled = BMUtil.GreedyFillMFG(m,container.Height(),container.Width());
      boolean res = BMUtil.HasDiagRect_naive(mfilled,container.Height(),container.Width());
      return !res ? "Works" : "Fails";
   }
}
