using System;
using System.Collections.Generic;
using System.Data.Entity;
using System.Linq;
using System.Web;
using System.ComponentModel.DataAnnotations.Schema;
using System.ComponentModel.DataAnnotations;

namespace BackendServer
{
    public class ApplicationDatabase : DbContext
    {
        public ApplicationDatabase() :
            base("DefaultConnection")
        {
        }

        public DbSet<HostIdentity> Hosts { get; set; }
    }

    public class HostIdentity
    {
        [Key]
        [DatabaseGeneratedAttribute(DatabaseGeneratedOption.Identity)]
        public int Id { get; set; }
        public string HostUniqueId { get; set; }
        public string HostCode { get; set; }
        public string Ip { get; set; }
    }
}