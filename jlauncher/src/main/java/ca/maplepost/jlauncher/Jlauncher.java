/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Project/Maven2/JavaApp/src/main/java/${packagePath}/${mainClassName}.java to edit this template
 */

package ca.maplepost.jlauncher;

import java.util.logging.Logger;

/**
 *
 * @author peterslack
 */
public class Jlauncher {

    private static native void shutdownHookCalled();
    private static final Logger LOG = Logger.getLogger(Jlauncher.class.getName());

    class Handler implements Thread.UncaughtExceptionHandler {

        public void uncaughtException(Thread t, Throwable e) {
            LOG.info("Unhandled exception caught!");
        }
    }

    public Jlauncher() {
        Handler globalExceptionHandler = new Handler();
        Thread.setDefaultUncaughtExceptionHandler(globalExceptionHandler);
    
        Runtime.getRuntime().addShutdownHook(new Thread() {
            @Override
            public void run() {
                shutdownHookCalled();
            }

        });

    }

}
