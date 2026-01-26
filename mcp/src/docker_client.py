import os
import time
import docker
from typing import Optional, List, Dict, Any
from docker.models.containers import Container
from docker.errors import DockerException, NotFound

from src.models.common import ExecutionResult, ExecutionStatus


class DarkmoonDockerClient:
    """
    Docker client to interact with the Darkmoon security toolbox container.
    Handles command execution, health checks, and resource management.
    """

    def __init__(
        self,
        container_name: str = "darkmoon",
        timeout: int = 300,
    ):
        """
        Initialize the Docker client.

        Args:
            container_name: Name of the Darkmoon container
            timeout: Default timeout for command execution (seconds)
        """
        self.container_name = container_name
        self.default_timeout = timeout
        try:
            self.client = docker.from_env()
        except DockerException as e:
            raise RuntimeError(f"Failed to connect to Docker: {e}")

    def get_container(self) -> Optional[Container]:
        """Get the Darkmoon container if it exists and is running."""
        try:
            container = self.client.containers.get(self.container_name)
            return container if container.status == "running" else None
        except NotFound:
            return None
        except DockerException as e:
            raise RuntimeError(f"Error accessing container: {e}")

    def execute_command(
        self,
        command: str | List[str],
        timeout: Optional[int] = None,
        workdir: Optional[str] = None,
        environment: Optional[Dict[str, str]] = None,
    ) -> ExecutionResult:
        """
        Execute a command inside the Darkmoon container.

        Args:
            command: Command to execute (string or list of args)
            timeout: Timeout in seconds (uses default if not specified)
            workdir: Working directory for command execution
            environment: Additional environment variables

        Returns:
            ExecutionResult with status, output, and metadata
        """
        container = self.get_container()
        if not container:
            return ExecutionResult(
                status=ExecutionStatus.FAILED,
                stderr=f"Container '{self.container_name}' not found or not running",
                exit_code=1,
            )

        timeout = timeout or self.default_timeout
        start_time = time.time()

        try:
            # Prepare command
            if isinstance(command, list):
                cmd = command
            else:
                cmd = ["bash", "-c", command]

            # Execute command
            exec_result = container.exec_run(
                cmd=cmd,
                workdir=workdir,
                environment=environment,
                demux=True,  # Separate stdout/stderr
            )

            duration = time.time() - start_time

            # Handle output
            stdout = exec_result.output[0].decode("utf-8") if exec_result.output[0] else ""
            stderr = exec_result.output[1].decode("utf-8") if exec_result.output[1] else ""

            status = (
                ExecutionStatus.SUCCESS
                if exec_result.exit_code == 0
                else ExecutionStatus.FAILED
            )

            return ExecutionResult(
                status=status,
                stdout=stdout,
                stderr=stderr,
                exit_code=exec_result.exit_code,
                duration=duration,
                metadata={
                    "command": command if isinstance(command, str) else " ".join(command),
                    "workdir": workdir,
                    "timeout": timeout,
                },
            )

        except Exception as e:
            duration = time.time() - start_time
            return ExecutionResult(
                status=ExecutionStatus.FAILED,
                stderr=f"Execution error: {str(e)}",
                exit_code=1,
                duration=duration,
                metadata={"command": str(command), "error": str(e)},
            )

    def check_tool_available(self, tool_name: str) -> bool:
        """
        Check if a tool is available in the container.

        Args:
            tool_name: Name of the tool to check

        Returns:
            True if tool is available, False otherwise
        """
        result = self.execute_command(f"which {tool_name}", timeout=5)
        return result.success

    def get_disk_usage(self) -> Optional[Dict[str, Any]]:
        """Get disk usage information from the container."""
        result = self.execute_command("df -h /opt/darkmoon/out", timeout=5)
        if result.success:
            # Parse df output
            lines = result.stdout.strip().split("\n")
            if len(lines) >= 2:
                parts = lines[1].split()
                return {
                    "filesystem": parts[0],
                    "size": parts[1],
                    "used": parts[2],
                    "available": parts[3],
                    "use_percent": parts[4],
                    "mounted_on": parts[5] if len(parts) > 5 else "-",
                }
        return None

    def health_check(self) -> Dict[str, Any]:
        """
        Perform a comprehensive health check.

        Returns:
            Dictionary with health status information
        """
        container = self.get_container()
        if not container:
            return {
                "healthy": False,
                "container_running": False,
                "message": f"Container '{self.container_name}' not found or not running",
            }

        # Check essential tools
        tools_to_check = ["naabu", "nuclei", "httpx", "subfinder"]
        tools_status = {}
        for tool in tools_to_check:
            tools_status[tool] = self.check_tool_available(tool)

        # Get disk usage
        disk_usage = self.get_disk_usage()

        all_tools_available = all(tools_status.values())

        return {
            "healthy": all_tools_available,
            "container_running": True,
            "tools_available": tools_status,
            "disk_usage": disk_usage,
            "message": "All systems operational"
            if all_tools_available
            else "Some tools are not available",
        }

    def cleanup(self):
        """Clean up Docker client resources."""
        if hasattr(self, "client"):
            self.client.close()
