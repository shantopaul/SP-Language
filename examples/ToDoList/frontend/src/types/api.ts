export interface FieldValidationError {
  field: string;
  code: string;
  message: string;
}

export interface ProblemDetails {
  type?: string;
  title: string;
  status: number;
  detail: string;
  instance?: string;
  request_id?: string;
  errors?: FieldValidationError[];
}

export class ApiError extends Error {
  public readonly status: number;
  public readonly title: string;
  public readonly detail: string;
  public readonly errors: FieldValidationError[];
  public readonly requestId?: string;

  constructor(problem: ProblemDetails) {
    super(problem.detail || problem.title || `HTTP ${problem.status}`);
    this.name = "ApiError";
    this.status = problem.status;
    this.title = problem.title;
    this.detail = problem.detail;
    this.errors = problem.errors ?? [];
    this.requestId = problem.request_id;
  }

  public fieldMessage(field: string): string | undefined {
    return this.errors.find((item) => item.field === field)?.message;
  }
}

export interface HealthResponse {
  status: string;
  service: string;
  version: string;
  database: string;
}
