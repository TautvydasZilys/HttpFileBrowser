using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Net.Sockets;
using System.Web;

namespace BackendServer.Models
{
    public class FileHostModel
    {
        [Required]
        [Display(Name = "File host code")]
        public string FileHostCode { get; set; }
    }

    public class HostIdentityModel
    {
        [Required]
        public string SystemUniqueId { get; set; }
        public Socket HostSocket { get; set; }
    }

    public class ClientIdentityModel
    {
        public string IpAddress { get; set; }
    }
}