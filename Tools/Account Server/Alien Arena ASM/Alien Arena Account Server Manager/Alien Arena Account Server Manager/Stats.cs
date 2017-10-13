﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.Data;
using System.Data.SqlClient;
using System.Net.Sockets;
using System.Net;
using System.Threading;
using System.Text;
using System.IO;
using System.Net.NetworkInformation;

namespace Alien_Arena_Account_Server_Manager
{
    public class Stats
    {
        public static bool getStats = false;
        public static bool uploadStats = false;
        public static playerList players = new playerList();
        public static rankingList allPlayers = new rankingList();
        public static int TotalPlayers = 0;
        public static int DMServers = 0;
        public static int CTFServers = 0;
        public static int TACServers = 0;

        public static List<int> ServerCount = new List<int>(new int[10]); //Servers
        public static List<int> PlayerCount = new List<int>(new int[10]); //Clients(all, including bots)
        public static List<int> ValidatedPlayerCount = new List<int>(new int[10]); //Validated players

        public class ServerList
        {
            public List<string> Ip;
            public List<ushort> Port;
            public List<string> Name;
            public List<string> Map;
            public List<string> Status;     

            public ServerList()
            {
                Ip = new List<string>();
                Port = new List<ushort>();
                Name = new List<string>();
                Map = new List<string>();
                Status = new List<string>();
           }

            public void Add(string Ip, ushort Port, string Name, string Map)
            {
                this.Ip.Add(Ip);
                this.Port.Add(Port);
                this.Name.Add(Name);
                this.Map.Add(Map);
                Status.Add("Active");
            }

            public void Clear()
            {
                Ip.RemoveAll(AllStrings);
                Port.RemoveAll(AllUShorts);
                Name.RemoveAll(AllStrings);
                Map.RemoveAll(AllStrings);
                Status.RemoveAll(AllStrings);
            }

            private static bool AllStrings(String s)
            {
                return true;
            }

            private static bool AllUShorts(ushort i)
            {
                return true;
            }
        }

        public static ServerList Servers = new ServerList();

        private bool VerifyPlayer(string Name)
        {
            pProfile Profile = DBOperations.CheckPlayer(Name);

            if (Profile.Status == "Active")
                return true;
            else
                return false;
        }

        private static double AverageFragrate()
        {
            double avg = 1.0f;

            for (int i = 0; i < players.name.Count; i++)
            {
                pProfile Profile = DBOperations.CheckPlayer(players.name[i]);
                avg += Profile.TotalFrags / (Profile.TotalTime > 0?Profile.TotalTime:1);
            }
            if (players.name.Count > 0)
                avg /= (double)players.name.Count;

            return Math.Round(avg, 3);
        }
        private static void ProcessPlayers()
        {
            //This is the meat of the program.
            //Run through the player's fragrates - weight the fragrate by the players standing in the game.  So a player ranked #1 would get more points than
            //someone at the bottom.  Should be by percentage, such as if there are 10 players, the guy at the top gets 100%, the next guy down 90%, and so on.
            //Then, weight the entire thing by the average cummulative fragrate of the players in the game.  

            double rFactor = AverageFragrate();

            //calcuate points for each player based on position and fragrate
            for(int i = 0; i < players.name.Count; i++)
            {
                int thisPos = 1;

                //Find the current, fragrate based position(we insert in order, so no sorting needed here)
                for (int j = 0; j < players.name.Count; j++)
                {
                    if (players.name[i] == players.name[j])
                    {
                        thisPos = j;
                        break;
                    }                   
                }

                double pFactor = 1.0f - (double)thisPos / (players.name.Count>0?players.name.Count:1);

                //Get this player's profile and add data to it
                pProfile Profile = DBOperations.CheckPlayer(players.name[i]);

                double pFragrate = Math.Round(players.frags[i] / (players.hour[i] * 60.0f + players.minutes[i] > 0 ? players.hour[i] * 60.0f + players.minutes[i] : 1), 3);

                //ACCServer.sDialog.UpdateStatus(Profile.Name + " nums - Pos: " + thisPos.ToString() + " pFactor:" + pFactor.ToString() + " rFactor" + rFactor.ToString() + " pFragrate" + pFragrate.ToString());
            
                Profile.StatPoints += Math.Round(pFragrate * pFactor * rFactor * 0.01f, 3);
                Profile.TotalFrags += players.frags[i]; //change to totalfrags throughout
                Profile.TotalTime += 1; //one minute per poll

                DBOperations.UpdatePlayer(Profile.Name, Profile.StatPoints.ToString(), Profile.TotalFrags.ToString(), Profile.TotalTime.ToString());
            }
        }

        private static void PingServers()
        {
            int i = 0;

            Servers.Clear();
            TotalPlayers = 0;
            DMServers = 0;
            CTFServers = 0;
            TACServers = 0;

            netStuff.sServer.RequestServerList();

            //Check for invalid list
            if (Servers.Ip.Count < 1)
                return;

            for(i = 0; i < Servers.Ip.Count; i++)
            {
                players.Clear();
                netStuff.sServer.GetServerInfo(Servers.Ip[i], Servers.Port[i], i);

                if(players.name.Count > 0 && Servers.Status[i] == "Active")
                    ProcessPlayers();
            }

            //Keep track of server counts
            ServerCount.Insert(0, Servers.Ip.Count);
            if (ServerCount.Count > 10)
                ServerCount.RemoveRange(10, ServerCount.Count - 10);

            //Keep track of total client counts
            PlayerCount.Insert(0, TotalPlayers);
            if (PlayerCount.Count > 10)
                PlayerCount.RemoveRange(10, PlayerCount.Count - 10);

            //Keep track of validated
            ValidatedPlayerCount.Insert(0, netStuff.players.name.Count);
            if (ValidatedPlayerCount.Count > 10)
                ValidatedPlayerCount.RemoveRange(10, ValidatedPlayerCount.Count - 10);

            //Update the graphic displays
            ACCServer.sDialog.UpdateServerChart();
            ACCServer.sDialog.UpdatePlayerChart();
            ACCServer.sDialog.UpdateServerTypeChart();

            ACCServer.sDialog.UpdateRankingList();
            ACCServer.sDialog.UpdateServers();
        }

        public static void StatsGen()
        {
            while (getStats == true)
            {
                if (CheckInternetConnection() == true)
                {
                    ACCServer.sDialog.UpdateStatus("Polling Servers...");
                    PingServers();
                }
                //Poll each minute
                Thread.Sleep(60000);
            }
        }
        static bool CheckInternetConnection()
        {
            bool success = false;
            using (Ping ping = new Ping())
            {
                try
                {
                    if (ping.Send("google.com", 2000).Status == IPStatus.Success)
                    {
                        success = true;
                    }
                }
                catch (PingException)
                {
                    success = false;
                }
            }
            return success;
        }

        public static void UploadStatsFile()
        {
            if (CheckInternetConnection() == true)
            {
                WebClient client = new WebClient();

                client.Credentials = new NetworkCredential("aastats", "password");
                client.UploadFile("ftp://martianbackup.com/playerrank.db", "STOR", "playerrank.db");

                ACCServer.sDialog.UpdateStatus("Upload DB Complete.");
            }
        }

        public static void UploadStats()
        {
            while (uploadStats == true)
            {
                //Upload each hour
                ACCServer.sDialog.UpdateStatus("Uploading Stats...");

                DBOperations.GenerateStatsFile();
                UploadStatsFile();
                Thread.Sleep(3600000);
            }
        }
    }
}