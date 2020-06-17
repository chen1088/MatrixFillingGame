package com.applications;

import javax.swing.*;
import java.util.List;
import java.util.concurrent.ExecutionException;

public abstract class MFGSwingWorker extends SwingWorker {
   public BMContainer container;
   public JTextField resultField;
   public JProgressBar progressBar;

   public MFGSwingWorker(BMContainer c, JTextField tf, JProgressBar pb)
   {
      super();
      container = c;
      resultField = tf;
      progressBar = pb;
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
         if(!isCancelled())resultField.setText(get().toString());
      } catch (InterruptedException e) {
         e.printStackTrace();
      } catch (ExecutionException e) {
         e.printStackTrace();
      }
      super.done();
   }
}

