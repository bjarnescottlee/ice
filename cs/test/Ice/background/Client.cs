// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

using System;

public class Client
{
    public static int run(string[] args, Ice.Communicator communicator)
    {
        Test.BackgroundPrx background = AllTests.allTests(communicator);
        background.shutdown();
        return 0;
    }

    public static void Main(string[] args)
    {
        int status = 0;
        Ice.Communicator communicator = null;

        try
        {
            Ice.InitializationData initData = new Ice.InitializationData();
            initData.properties = Ice.Util.createProperties(ref args);

            //
            // For this test, we want to disable retries.
            //
            initData.properties.setProperty("Ice.RetryIntervals", "-1");

            //
            // This test kills connections, so we don't want warnings.
            //
            initData.properties.setProperty("Ice.Warn.Connections", "0");

            //
            // Setup the test transport plugin.
            //
            string defaultProtocol = initData.properties.getPropertyWithDefault("Ice.Default.Protocol", "tcp");
            initData.properties.setProperty("Ice.Default.Protocol", "test-" + defaultProtocol);

            communicator = Ice.Util.initialize(ref args, initData);
            PluginI plugin = new PluginI(communicator);
            plugin.initialize();
            communicator.getPluginManager().addPlugin("Test", plugin);

            status = run(args, communicator);
        }
        catch(Ice.LocalException ex)
        {
            Console.Error.WriteLine(ex);
            status = 1;
        }

        if(communicator != null)
        {
            try
            {
                communicator.destroy();
            }
            catch(Ice.LocalException ex)
            {
                Console.Error.WriteLine(ex);
                status = 1;
            }
        }

        if(status != 0)
        {
            System.Environment.Exit(status);
        }
    }
}