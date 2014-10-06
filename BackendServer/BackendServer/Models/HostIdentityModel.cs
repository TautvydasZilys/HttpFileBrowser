using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Web;

namespace BackendServer.Models
{
    public class HostIdentityModel
    {
        [Required]
        public string SystemUniqueId { get; set; }
        public string EndPoint { get; set; }
    }
}