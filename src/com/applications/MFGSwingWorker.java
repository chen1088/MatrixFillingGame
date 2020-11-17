package com.applications;

import javax.swing.*;
import java.util.List;
import java.util.concurrent.ExecutionException;

public abstract class MFGSwingWorker extends SwingWorker<String,Object> {
   public BMContainer container;
   public JTextPane resultField;
   public JProgressBar progressBar;
   public JCheckBox enable;
   public JPanel subPanel;

   public MFGSwingWorker(BMContainer c, JTextPane tf, JProgressBar pb, JCheckBox cb,JPanel rp)
   {
      super();
      container = c;
      resultField = tf;
      progressBar = pb;
      enable = cb;
      subPanel = rp;
   }
   public MFGSwingWorker(BMContainer c)
   {
      super();
      container = c;
   }
   public void Publish(int percent)
   {
      publish(percent);
   }

   @Override
   protected void process(List chunks) {
      int p = (int)chunks.get(chunks.size() -1);
      progressBar.setValue(p);
      super.process(chunks);
   }

   @Override
   protected void done() {
      try {
         if(!isCancelled())
            if(resultField != null)
               resultField.setText(get());
      } catch (InterruptedException e) {
         e.printStackTrace();
      } catch (ExecutionException e) {
         e.printStackTrace();
      }
      super.done();
   }

   public abstract MFGSwingWorker Copy();
   public abstract void SetAppearance();
}

